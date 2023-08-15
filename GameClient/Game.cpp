#include "Game.hpp"
#include "cl_Chat.hpp"
#include <Windows.h>
#include <tlhelp32.h>

bool AlreadyRunning(const char* processName)
{
    PROCESSENTRY32 procEntry32;
    procEntry32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcSnap == INVALID_HANDLE_VALUE)
        return false;

    while (Process32Next(hProcSnap, &procEntry32))
    {
        if (procEntry32.th32ProcessID == GetCurrentProcessId()) continue;
#ifdef _WIN64
        static char szstr[256];
        ZeroMemory(szstr, 256);
        size_t unused;
        wcstombs_s(&unused, szstr, procEntry32.szExeFile, 256);
        if (!strcmp(processName, szstr))
#else
        if (!strcmp(processName, procEntry32.szExeFile))
#endif
        {
            CloseHandle(hProcSnap);
            return true;
        }

    }

    CloseHandle(hProcSnap);
    return false;
}

Game::Game() : window(sf::VideoMode(1920, 1080), "XerxesGames")
{
    if (AlreadyRunning("GameClient.exe")) window.setPosition(sf::Vector2i(1920, 0));
    else window.setPosition(sf::Vector2i(0, 0));

    view.setSize(g_CamSizeX, g_CamSizeY);
    view.setCenter(g_CamSizeX / 2.f, g_CamSizeY / 2.f);
    window.setView(view);

    LogSystem::Get().Init(&this->window, &m_fDeltaTime);
    LeaderBoard::Get().Init(&m_Font);
    Killfeed::Get().Init(&this->window, &m_fDeltaTime);

    window.setFramerateLimit(60);
    m_Font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf");

    m_ConnectingText.setFont(m_Font);
    m_ConnectingText.setCharacterSize(50);
    m_ConnectingText.setString("Connecting...");
    m_ConnectingText.setFillColor(sf::Color::Blue);

    m_GameOverText.setFont(m_Font);
    m_GameOverText.setCharacterSize(80);
    m_GameOverText.setString("Y r gay");
    m_GameOverText.setFillColor(sf::Color::Yellow);

    m_pPlayers = std::make_shared<Players>(window, m_Font, m_fDeltaTime);
    m_pGameNetwork = std::make_shared<GameNetwork>(m_pPlayers, m_fDeltaTime);
    m_pUI = std::make_shared<UI>(&window);

    m_pLogin = std::make_unique<Login>(window, m_pGameNetwork);

    //m_mapPlayers[0].playerInfo.n_uID = 0;
    //m_mapPlayers[0].playerInfo.v_fPos = { 10.f, 10.f };
}

Game::~Game()
{
}

bool Game::Connect()
{
    return m_pGameNetwork->ConnectToServer();
}

void Game::MainLoop()
{
    Startup();

    while (window.isOpen())
    {
        HandleEvents();
        UpdateClient();
        DrawFrame();
    }
}

void Game::Startup()
{
    Connect();

    if (m_pLogin->MainLoop()) return;
    m_pGameNetwork->RegisterClient();

    while (!m_pGameNetwork->LocalPlayerAdded() && !g_bOfflinePLay)
    {
        m_pGameNetwork->ProcessIncomingPackets();
    }

    AudioManager::Get().PlayGameSound(GameSounds::JOIN, 60.f);
    AudioManager::Get().PlayGameMusic(GameMusics::INGAME, 20.f);
    m_pUI->Init(m_pPlayers->GetLocalPLayerStat());
    Chat::Get().Init(&window, m_pPlayers->GetLocalPLayer(), m_pGameNetwork, &m_fDeltaTime, &m_PressedKeyCodes);
}

void Game::Disconnected()
{
    if (!window.isOpen()) return;
    LeaderBoard::Get().Reset();
    Chat::Get().Reset();

    m_pPlayers.reset();
    m_pUI.reset();
    m_pGameNetwork.reset();

    m_pPlayers = std::make_shared<Players>(window, m_Font, m_fDeltaTime);
    m_pGameNetwork = std::make_shared<GameNetwork>(m_pPlayers, m_fDeltaTime);
    m_pUI = std::make_shared<UI>(&window);

    Startup();
}

void Game::UpdateClient()
{
    if (!m_pGameNetwork->IsConnected()) {
        Disconnected();
        return;
    }

    m_fDeltaTime = m_DeltaClock.restart().asSeconds();  //needs to be cloese to each other bc the server interpolation
    g_CurrentTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    m_pPlayers->m_DeltaTimeTimePoint = g_CurrentTime;
    //std::cout << "deltaTime: " << m_fDeltaTime << '\n';

    if (!g_bOfflinePLay) m_pGameNetwork->ProcessIncomingPackets();

    AudioManager::Get().Update();
    Chat::Get().Update();
    m_pPlayers->Update();
    m_pUI->Update();
    if (g_bNeedUiUpdate) {
        m_pUI->UpdateName(); 
        m_pUI->UpdateFlagScore(); 
        g_bNeedUiUpdate = false;
    }
    if(!g_bOfflinePLay) m_pGameNetwork->SendPlayerStatusFixedRate();
}

void Game::DrawFrame()
{
    window.clear();
    if (m_bGameOver) {
        window.draw(m_GameOverText);
    }

    if (!m_pGameNetwork->Connected() && !g_bOfflinePLay) {
        window.draw(m_ConnectingText);
    }
    window.draw(*m_pPlayers);
    window.draw(*m_pUI);
    window.draw(Chat::Get());
    window.display();
}

void Game::HandleEvents()
{
    while (window.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed: m_pGameNetwork->UnRegisterClient(); window.close(); break;
        case sf::Event::MouseMoved:
        {
            //m_pPlayers->SetMouseMoved(true);
            break;
        }
        case sf::Event::KeyPressed:
            {
                switch (event.key.code)
                {
                case sf::Keyboard::Up: m_PressedKeyCodes.push_front(1001); break;
                case sf::Keyboard::Down: m_PressedKeyCodes.push_front(1002); break;
                default: break;
                }

                if (Chat::Get().IsActive()) break;
                switch (event.key.code)
                {
                case sf::Keyboard::I: m_pPlayers->EnableInterpolation(); break;
                case sf::Keyboard::C: m_pGameNetwork->PushFlagDropAction(); break;
                case sf::Keyboard::R: m_pPlayers->ChangeMouseRelative(); break;
                default: break;
                }
            }break;
        case sf::Event::TextEntered:
        {
            m_PressedKeyCodes.push_front(event.text.unicode);
        }break;
        case sf::Event::Resized:
            {
                static sf::Vector2u prevWindowSize{ 0u, 0u };
                if (prevWindowSize.x != window.getSize().x) {
                    window.setSize(sf::Vector2u(window.getSize().x, window.getSize().x * (9.f / 16.f)));
                    prevWindowSize = window.getSize();
                }
                else {
                    window.setSize(sf::Vector2u(window.getSize().y * (16.f / 9.f), window.getSize().y));
                    prevWindowSize = window.getSize();
                }

                //view.setSize(g_CamSizeY * ((float)window.getSize().x / (float)window.getSize().y), g_CamSizeY);
                //view.setCenter((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f);
                //window.setView(view);
            }
            break;
        default:break;
        }
    }
}

void Game::UpdateOptions()
{
    
}

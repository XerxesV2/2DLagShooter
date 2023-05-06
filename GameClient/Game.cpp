#include "Game.hpp"
#include "LogSystem.hpp"
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
        if (!strcmp(processName, procEntry32.szExeFile))
        {
            CloseHandle(hProcSnap);
            return true;
        }

    }

    CloseHandle(hProcSnap);
    return false;
}

Game::Game() : window(sf::VideoMode(1920, 1080), "gamegame")
{
    if (AlreadyRunning("GameClient.exe")) window.setPosition(sf::Vector2i(1920, 0));
    else window.setPosition(sf::Vector2i(0, 0));

    view.setSize(1920.f, 1080.f);
    view.setCenter(1920.f / 2.f, 1080.f / 2.f);
    window.setView(view);

    LogSystem::Get().Init(&this->window);

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
    while (window.isOpen())
    {
        HandleEvents();
        UpdateClient();
        DrawFrame();
    }
}

void Game::UpdateClient()
{
    if (m_bGameOver) return;

    if(!g_bOfflinePLay) m_pGameNetwork->ProcessIncomingPackets();
    if (!m_pGameNetwork->Connected() && !g_bOfflinePLay) return;

    m_fDeltaTime = m_DeltaClock.restart().asSeconds();  //needs to be cloese to each other bc the server interpolation
    m_pPlayers->m_DeltaTimeTimePoint = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    //std::cout << "deltaTime: " << m_fDeltaTime << '\n';
    
    m_pPlayers->Update();
    if(!g_bOfflinePLay) m_pGameNetwork->SendPlayerStatusFixedRate();
    LogSystem::Get().Update();

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
    window.draw(LogSystem::Get());
    window.display();
}

void Game::HandleEvents()
{
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            window.close();
        if (event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
            case sf::Keyboard::I: m_pPlayers->EnableInterpolation(); break;
            default: break;
            }
        }
        if (event.type == sf::Event::Resized)
        {
            //window.setView(view);
        }
    }
}

void Game::UpdateOptions()
{
    
}

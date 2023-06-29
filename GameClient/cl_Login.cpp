#include "cl_Login.hpp"
#include "cl_Globals.hpp"

Login::Login(sf::RenderWindow& window, std::shared_ptr<GameNetwork>& network) : m_Window(window), m_pNetwork(network)
{
    m_Font.loadFromFile("fonts\\Roboto-Medium.ttf");

    m_InfoText.setFont(m_Font);
    m_InfoText.setFillColor(sf::Color::Green);
    m_InfoText.setCharacterSize(80);
    m_InfoText.setString("Waiting for the server to accept...");
    m_InfoText.setOrigin(m_InfoText.getGlobalBounds().width / 2.f, m_InfoText.getGlobalBounds().height / 2.f);
    m_InfoText.setPosition(m_Window.getSize().x / 2.f, m_Window.getSize().y / 4.f);
    
    m_ErrorText.setFont(m_Font);
    m_ErrorText.setFillColor(sf::Color::Red);
    m_ErrorText.setCharacterSize(80);
    m_ErrorText.setString("");
    m_ErrorText.setOrigin(m_ErrorText.getGlobalBounds().width / 2.f, m_ErrorText.getGlobalBounds().height / 2.f);
    m_ErrorText.setPosition(m_Window.getSize().x / 2.f, m_Window.getSize().y / 4.f);

    
    m_pTextBox = std::make_unique<TextInputBox>(m_Font, m_Window, m_PressedKeyCodes);
    m_pTextBox->SetMaxInputLegth(20);
    m_pTextBox->SetSize({500.f, 100.f});
    m_pTextBox->SetPos(sf::Vector2f(m_Window.getSize())/2.f);

    std::srand(std::time(0));
}

Login::~Login()
{
}

int Login::MainLoop()
{
    if (g_bOfflinePLay) return 0;

    if(!m_pNetwork->Connected())
        SetString(m_InfoText, ""), SetString(m_ErrorText, "Server is down");

    while (m_Window.isOpen())   //kikattint = crash
    {
        HandleEvents();

        if (g_bAutoLogin && !m_bWaitForLoginResult && m_pNetwork->Connected()) {
            std::string generatedName("kenderpok" + std::to_string(std::rand() % 9998 + 1));
            std::memcpy(m_LoginInfo.sz_unsername, generatedName.c_str(), generatedName.size());
            m_pNetwork->SendLoginInformation(m_LoginInfo);
            SetString(m_ErrorText, ""); SetString(m_InfoText, "Waiting for login response...");
            m_bWaitForLoginResult = true;
        }

        if (m_pTextBox->Update() && !m_bWaitForLoginResult && m_Window.hasFocus() && m_pNetwork->Connected()) {
            std::memcpy(m_LoginInfo.sz_unsername, m_pTextBox->GetText().c_str(), m_pTextBox->GetText().size());
            m_pNetwork->SendLoginInformation(m_LoginInfo);
            SetString(m_ErrorText, ""); SetString(m_InfoText, "Waiting for login response...");
            m_bWaitForLoginResult = true;
        }

        switch (m_pNetwork->WaitForLoginReply())
        {
        case GameMessages::LoginAccept: m_bWaitForLoginResult = false; return 0;
        case GameMessages::LoginRefuse: m_bWaitForLoginResult = false; SetString(m_InfoText, ""); SetString(m_ErrorText, "Failed to log in. REFUSED"); break;
        case GameMessages::ServerRefuse: SetString(m_InfoText, ""); SetString(m_ErrorText, "Failed to connect to the server. REFUSED"); DrawFrame(); Sleep(1000); return 1;
        case GameMessages::ServerAccept: SetString(m_InfoText, ""); SetString(m_ErrorText, "");
        case GameMessages::None: break;
        default: break;
        }

        DrawFrame();
    }
	return 1;
}

void Login::HandleEvents()
{
    while (m_Window.pollEvent(m_Event))
    {
        switch (m_Event.type)
        {
        case sf::Event::Closed: m_Window.close(); break;
        case sf::Event::TextEntered:
        {
            m_PressedKeyCodes.push_front(m_Event.text.unicode);
        }break;
        case sf::Event::Resized:
        {
            static sf::Vector2u prevWindowSize{ 0u, 0u };
            if (prevWindowSize.x != m_Window.getSize().x) {
                m_Window.setSize(sf::Vector2u(m_Window.getSize().x, m_Window.getSize().x * (9.f / 16.f)));
                prevWindowSize = m_Window.getSize();
            }
            else {
                m_Window.setSize(sf::Vector2u(m_Window.getSize().y * (16.f / 9.f), m_Window.getSize().y));
                prevWindowSize = m_Window.getSize();
            }
        }
        default:break;
        
        }
    }
}

void Login::DrawFrame()
{
    m_Window.clear();

    if (m_pNetwork->Accepted())
        m_Window.draw(*m_pTextBox);

    m_Window.draw(m_InfoText);
    m_Window.draw(m_ErrorText);

    m_Window.display();
}

void Login::SetString(sf::Text& text, std::string_view str)
{
    text.setString(str.data());
    text.setOrigin(text.getGlobalBounds().width / 2.f, text.getGlobalBounds().height / 2.f);
    text.setPosition(m_Window.getSize().x / 2.f, m_Window.getSize().y / 4.f);
}

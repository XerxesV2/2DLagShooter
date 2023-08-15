#include "cl_Login.hpp"
#include "cl_Globals.hpp"
#include "utils.hpp"

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

    TryToLogIn();

    while (m_Window.isOpen())
    {
        HandleEvents();

		if (!m_pNetwork->Connected()) {
			SetString(m_InfoText, ""), SetString(m_ErrorText, "Server is down");
		}

        if (m_pTextBox->Update() && !m_bWaitForLoginResult && m_Window.hasFocus() && m_pNetwork->Connected()) {
            if (m_bAutoLoginSuccess) { TryToLogIn(); }
            else
            {
                std::memcpy(m_LoginInfo.sz_unsername, m_pTextBox->GetText().c_str(), m_pTextBox->GetText().size());
                std::memcpy(m_LoginInfo.sz_hwid, GetHWID(), HW_PROFILE_GUIDLEN);
                m_pNetwork->SendLoginInformation(m_LoginInfo);
                SetString(m_ErrorText, ""); SetString(m_InfoText, "Waiting for login response...");
                m_bWaitForLoginResult = true;
                m_pTextBox->SetActive(false);
            }
        }

        switch (m_pNetwork->WaitForLoginReply())
        {
        case GameMessages::MysteriousError: m_bWaitForLoginResult = false; SetString(m_ErrorText, "Mysterious Error"); SetString(m_InfoText, ""); break;
        case GameMessages::AutoLoginFail: m_bWaitForLoginResult = false; m_bAutoLoginSuccess = false; SetString(m_ErrorText, "Auto login failed no registered user found"); SetString(m_InfoText, ""); break;
        case GameMessages::AutoLoginSuccess: m_bWaitForLoginResult = false; m_bAutoLoginSuccess = true; return 0;
        case GameMessages::LoginAccept: m_bWaitForLoginResult = false; return 0;
        case GameMessages::LoginRefuse: m_bWaitForLoginResult = false; SetString(m_InfoText, ""); SetString(m_ErrorText, "Failed to log in. REFUSED"); break;
        case GameMessages::ServerRefuse: m_bWaitForLoginResult = false; SetString(m_InfoText, ""); SetString(m_ErrorText, "Failed to connect to the server. REFUSED"); break;
        case GameMessages::ServerAccept: SetString(m_InfoText, "Loading..."); SetString(m_ErrorText, ""); break;
        case GameMessages::BannedClient:
        {
            std::string str("You are banned till: ");
            str.append(Utils::TimeToString(m_pNetwork->GetBanTime() == INT_MAX ? std::numeric_limits<long long>::max() / 1000000000ll : m_pNetwork->GetBanTime()));
            SetString(m_InfoText, ""); SetString(m_ErrorText, str); m_bWaitForLoginResult = false;
        }break;
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
        case sf::Event::Closed: m_pNetwork->UnRegisterClient(); m_Window.close(); break;
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
        } break;
        default:break;
        
        }
    }
}

void Login::TryToLogIn()
{
    std::memset(m_LoginInfo.sz_unsername, 0, sizeof(m_LoginInfo.sz_unsername));
    std::memcpy(m_LoginInfo.sz_hwid, GetHWID(), HW_PROFILE_GUIDLEN);
    m_pNetwork->SendAutoLoginInformation(m_LoginInfo);
    SetString(m_ErrorText, ""); SetString(m_InfoText, "Waiting for auto login response...");
    m_bWaitForLoginResult = true;
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

const char* Login::GetHWID()
{
    static HW_PROFILE_INFO hwProfileInfo;
    if (GetCurrentHwProfile(&hwProfileInfo))
        printf("HWID: %ws\n", hwProfileInfo.szHwProfileGuid);

#ifdef _WIN64
    static char szstr[HW_PROFILE_GUIDLEN];
    ZeroMemory(szstr, HW_PROFILE_GUIDLEN);
    size_t unused;
    wcstombs_s(&unused, szstr, hwProfileInfo.szHwProfileGuid, HW_PROFILE_GUIDLEN);
    return szstr;
#else
    return hwProfileInfo.szHwProfileGuid;
#endif
}

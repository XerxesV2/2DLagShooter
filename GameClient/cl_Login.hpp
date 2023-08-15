#pragma once
#include "UI/TextInputBox.hpp"
#include "cl_GameNetwork.hpp"


class Login
{
public:
	Login(sf::RenderWindow& window, std::shared_ptr<GameNetwork>& network);
	~Login();

	int MainLoop();

private:
	void HandleEvents();
	void TryToLogIn();
	void DrawFrame();
	void SetString(sf::Text& text, std::string_view str);
	const char* GetHWID();

private:
	sf::RenderWindow& m_Window;
	std::shared_ptr<GameNetwork>& m_pNetwork;
	std::unique_ptr<TextInputBox> m_pTextBox;

	sf::Font m_Font;
	sf::Text m_InfoText;
	sf::Text m_ErrorText;
	sf::Event m_Event;
	std::deque<uint32_t> m_PressedKeyCodes;

	shared::LoginInformation m_LoginInfo;

	bool m_bWaitForLoginResult = false;
	bool m_bAutoLoginSuccess = false;
};


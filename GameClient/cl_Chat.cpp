#include "cl_Chat.hpp"
#include "cl_GameNetwork.hpp"
#include "PlayerStruct.hpp"

Chat::Chat() : m_ChatMessage(m_Font, 40)
{
	m_Font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf");
}

Chat::~Chat()
{
}

void Chat::Init(sf::RenderWindow* window, PlayerStruct* pLocalPlayer, std::shared_ptr<GameNetwork>& network, float* _pfDeltaTime, std::deque<uint32_t>* keyCodes)
{
	m_pWindow = window;
	m_pLocalPlayer = pLocalPlayer;
	m_pfDeltaTime = _pfDeltaTime;
	m_pNetwork = network;
	m_pTextBox = std::make_unique<TextInputBox>(m_Font, *m_pWindow, *keyCodes);
	sf::Vector2f size{ 600.f, 60.f };
	m_pTextBox->SetSize(size);
	m_pTextBox->SetPos({ size.x / 2.f + 20.f, g_CamSizeY - 100.f });
	m_pTextBox->SetCharacterSize(36);
	m_pTextBox->SetOutlineThickness(3.f);
	m_pTextBox->SetTextOutlineThickness(2.f);
	m_pTextBox->SetHintText("Chat");
	m_pTextBox->SetMaxInputLegth(128u);
	m_pTextBox->SetUnicodeBounds(31, 127);
	m_pTextBox->SetAutoComplete(&m_AutoCompleteCommands);
	m_pTextBox->SetNameList(&m_NameList);

	GlobChat::m_MessagesStartPos = { 0.f, m_pTextBox->GetGlobalBounds().top };
	GlobChat::m_MessageHight = m_ChatMessage.textTemplate.getGlobalBounds().height;

	SetForeground(false);
}

void Chat::Update()
{
	static bool enterHold = false;

	if (m_pTextBox->Update() && m_pWindow->hasFocus() && !enterHold) {
		std::string& msg = m_pTextBox->GetText();
		if (!msg.empty())
		{
			if (msg.front() == '/')
			{
				HandleUserCommand(msg);
			}
			else
			{
				m_pNetwork->SendChatMessage(msg.c_str(), msg.size());
				AppendMessage(m_pLocalPlayer, msg.c_str());
			}
		}
		m_pTextBox->Clear();
		m_pTextBox->SetActive(false);
		enterHold = true;
		SetForeground(false);
	}
	else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
		enterHold = false;
	}

	if (m_pWindow->hasFocus() && sf::Keyboard::isKeyPressed(sf::Keyboard::T) && !m_pTextBox->IsActive()) {
		m_pTextBox->Clear();
		m_pTextBox->SetActive(true);
		SetForeground(true);
	}
	else if (sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
		SetForeground(m_pTextBox->IsActive());
	}

	UpdateMessages();
}

void Chat::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	for (auto& log : m_ChatMessages) {
		for (auto& text : log.first.texts)
			target.draw(text);
	}

	target.draw(*m_pTextBox);
}

void Chat::AppendMessage(PlayerStruct* pPlayer, const char* message)
{
	if (m_ChatMessages.size() >= m_MaxMessages)
		m_ChatMessages.pop_front();

	//sf::Color(235, 9, 9, 255) admin

	static sf::Color nameColor;
	if (pPlayer != nullptr)
		nameColor = pPlayer->stats.teammate ? sf::Color(22, 242, 2, 255) : sf::Color(5, 153, 245, 255);
	else
		nameColor = sf::Color::Magenta;

	nameColor.a = 0;

	m_ChatMessage.texts.clear();
	if (pPlayer != nullptr)
		m_ChatMessage.Append((std::string("[") + pPlayer->stats.username + "]:").c_str());
	else
		m_ChatMessage.Append("[SERVER]:");
	m_ChatMessage.SetFillColorBack(nameColor);
	for (auto& log : m_ChatMessages)
		log.first.Move(0.f, -(GlobChat::m_AppendOffsetY + GlobChat::m_SpaceingY));
	m_ChatMessages.push_back(std::make_pair(m_ChatMessage, m_Clock.getElapsedTime()));

	/*same line*/
	m_ChatMessages.back().first.Append(message);
	m_ChatMessages.back().first.SetFillColorBack(sf::Color::White);

	SetForeground(m_pTextBox->IsActive());
}

void Chat::AppendPlainMessage(const char* message, sf::Color& color)
{
	if (m_ChatMessages.size() >= m_MaxMessages)
		m_ChatMessages.pop_front();

	color.a = 0;

	m_ChatMessage.texts.clear();
	m_ChatMessage.Append(message);
	m_ChatMessage.SetFillColorBack(color);
	for (auto& msg : m_ChatMessages)
		msg.first.Move(0.f, -(GlobChat::m_AppendOffsetY + GlobChat::m_SpaceingY));
	m_ChatMessages.push_back(std::make_pair(m_ChatMessage, m_Clock.getElapsedTime()));

	SetForeground(m_pTextBox->IsActive());
}

void Chat::AppendPlainMessage(const char* message, sf::Color&& color)
{
	if (m_ChatMessages.size() >= m_MaxMessages)
		m_ChatMessages.pop_front();

	color.a = 0;

	m_ChatMessage.texts.clear();
	m_ChatMessage.Append(message);
	m_ChatMessage.SetFillColorBack(color);
	for (auto& msg : m_ChatMessages)
		msg.first.Move(0.f, -(GlobChat::m_AppendOffsetY + GlobChat::m_SpaceingY));
	m_ChatMessages.push_back(std::make_pair(m_ChatMessage, m_Clock.getElapsedTime()));

	SetForeground(m_pTextBox->IsActive());
}

void Chat::AddPlayerNameToPlayerList(uint32_t id, const std::string& name)
{
	m_NameList.insert_or_assign(name, id);
}

void Chat::RemovePlayerFromPlayerList(const std::string& name)
{
	m_NameList.erase(name);
}

void Chat::UpdateMessages()
{
	uint8_t popAmount = 0;
	for (auto& [log, time] : m_ChatMessages) {

		if (m_Clock.getElapsedTime().asSeconds() < time.asSeconds() + GlobChat::m_MessageFadeInTime) {
			log.animPos = std::min(1.f, log.animPos + *m_pfDeltaTime);
			float easeOut = (1.f - std::powf(1.f - log.animPos, 4.f));

			for (auto& text : log.texts) {
				sf::Color c = text.getFillColor();
				c.a = (sf::Uint8)(log.aplha * easeOut);
				text.setFillColor(c);
			}
			log.SetPosition((GlobChat::m_MessagesStartPos.x + 30.f) * easeOut, log.texts.back().getPosition().y);
		}
	}
}

bool is_number(const std::string& s, bool neg)
{
	return !s.empty() && std::find_if(s.begin() + neg,
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

void Chat::HandleUserCommand(std::string& strcmd)
{
	shared::UserCommand cmd;
	uint32_t spaces = 0;
	size_t offset = 0;
	std::string sub;

	while (offset != std::string::npos && spaces != 2)
	{
		sub = strcmd.substr(1, strcmd.find(' ', offset+1)-1);
		offset = strcmd.find(' ', offset+1);
		++spaces;
		if (m_Commands.count(sub)) break;
	}

	auto get_next_param = [&]()
	{
		sub = strcmd.substr(offset + 1, strcmd.find(' ', offset + 1) - offset - 1);
		offset = strcmd.find(' ', offset + 1);
	};

	do
	{
		if (!m_Commands.count(sub)) break;
		
		cmd.uUserCommand = m_Commands[sub];

		switch (cmd.uUserCommand)
		{
		case shared::UserCmd::ADD_AMMO:
		case shared::UserCmd::ADD_HEALTH:
		{
			get_next_param();
			if (offset != std::string::npos && m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage("Name not found", sf::Color(122, 95, 48, 255)); return;
			}
			get_next_param();
			bool neg = false;
			if(!sub.empty())
				 neg = (sub.front() == '-' ? 1 : 0);
			if (is_number(sub, neg))
			{
				if(neg) sub.erase(0, 1);
				cmd.iValue = std::stoi(sub) * (neg ? -1 : 1);
			}
			else {
				AppendPlainMessage("No number specified", sf::Color(122, 95, 48, 255)); return;
			}
		} break;

		case shared::UserCmd::KILL:
		{
			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];

			cmd.iValue = -9999;

		} break;

		case shared::UserCmd::TELEPORT:
		{
			get_next_param();
			if (offset != std::string::npos && m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage("First name not found", sf::Color(122, 95, 48, 255)); return;
			}

			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDRight = m_NameList[sub];
			else {
				AppendPlainMessage("Secound name not found", sf::Color(122, 95, 48, 255)); return;
			}

		} break;

		case shared::UserCmd::HELP:
		default: 
		{
			AppendPlainMessage("/add [ammo/health] <player> [amount]\n"
								"/kill <player>\n"
								"/ban <player> [time in sec]\n"
								"/tp <player> <player>"
								, sf::Color(122, 110, 60, 255)); return;
		}break;
		}
		
		m_pNetwork->SendChatUserCommand(cmd);
		return;
		
	} while (0);

	AppendPlainMessage("Unknown command", sf::Color(122, 95, 48, 255));
	

}

void Chat::SetForeground(bool foreground)
{
	auto a = foreground ? 255 : 150;
	for (auto& message : m_ChatMessages)
	{
		message.first.SetFillColorA(a);
		message.first.aplha = a;
	}
	m_pTextBox->SetAlpha(a);
}

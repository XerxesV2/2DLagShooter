#include "cl_Chat.hpp"
#include "cl_GameNetwork.hpp"
#include "AudioManager.hpp"
#include "PlayerStruct.hpp"
#include "utils.hpp"

#include <filesystem>

Chat::Chat() : m_ChatMessage(m_Font, 34)
{
	m_Font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf");

	/*for (size_t i = 0; i < sizeof(shared::RankNames) / sizeof(shared::RankNames[0]); i++)
	{
		m_RankIdMap.insert({ shared::RankNames[i], (shared::PlayerRank)i });
	}*/

	std::string path = "sounds\\bloodpressure";
	uint32_t i = (uint32_t)GameSounds::END;
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		m_SoundNames.insert({ entry.path().stem().string(), i});
		++i;
	}
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

void Chat::Reset()
{
	m_ChatMessages.clear();
	m_NameList.clear();
	m_pTextBox->Clear();
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
				if (msg.size() < maxChatMessageLength)
					m_pNetwork->SendChatMessage(msg.c_str(), msg.size());
				else
					AppendPlainMessage(sf::Color(122, 95, 48, 255), "Message too big");
				//AppendMessage(m_pLocalPlayer, msg.c_str());
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

void Chat::AppendMessage(PlayerStruct* pPlayer, const ChatMessageData* message)
{
	if (m_ChatMessages.size() >= m_MaxMessages)
		m_ChatMessages.pop_front();

	//sf::Color(235, 9, 9, 255) admin

	static sf::Color teamColor;
	if (pPlayer != nullptr) {
		teamColor = pPlayer->stats.team == m_pLocalPlayer->stats.team ? sf::Color(22, 242, 2, 255) : sf::Color(5, 153, 245, 255);
		AudioManager::Get().PlayGameSound(GameSounds::CHAT_MESSAGE, 50.f);
		FindMagicWordInChatMessage(message->msg);
	}
	else
		teamColor = sf::Color::Magenta;

	teamColor.a = 0;

	m_ChatMessage.Clear();

	switch (message->fMsgFlags)
	{
	case (BYTE)FLAGS_CHATMSG::client_msg:
		m_ChatMessage.Append(("(" + Utils::CurrentTimeToShortString() + ")").c_str());
		m_ChatMessage.SetFillColorBack(teamColor);
		m_ChatMessage.Append(pPlayer->stats.displayName.c_str());
		m_ChatMessage.SetFillColorBack(m_RankColorMap[pPlayer->stats.rank]);
		m_ChatMessage.Append(":");
		m_ChatMessage.SetFillColorBack(sf::Color(100,100,100,255));
		break;
	case (BYTE)FLAGS_CHATMSG::private_msg:
		m_ChatMessage.Append(("(" + Utils::CurrentTimeToShortString() + ")").c_str());
		m_ChatMessage.SetFillColorBack(teamColor);
		m_ChatMessage.Append(pPlayer->stats.displayName.c_str());
		m_ChatMessage.SetFillColorBack(m_RankColorMap[pPlayer->stats.rank]);
		m_ChatMessage.Append("=>");
		m_ChatMessage.SetFillColorBack(sf::Color(2, 170, 242, 255));
		break;
	case (BYTE)FLAGS_CHATMSG::moderate_msg:
	case (BYTE)FLAGS_CHATMSG::server_msg:
		m_ChatMessage.Append("[SERVER]:"); 
		m_ChatMessage.SetFillColorBack(teamColor);
		break;
	default: break;
	}
	/*same line*/
	m_ChatMessage.Append(message->msg);
	m_ChatMessage.SetFillColorBack(sf::Color(212, 212, 212));

	for (auto& log : m_ChatMessages)
		log.first.Move(0.f, -(GlobChat::m_AppendOffsetY + GlobChat::m_SpaceingY));
	m_ChatMessages.push_back(std::make_pair(m_ChatMessage, m_Clock.getElapsedTime()));


	SetForeground(m_pTextBox->IsActive());
}

void Chat::AppendPlainMessage(sf::Color& color, const char* message, ...)
{
	char formattedMsgBuffer[521];
	va_list args;
	va_start(args, message);
	vsnprintf(formattedMsgBuffer, sizeof(formattedMsgBuffer), message, args);
	va_end(args);

	if (m_ChatMessages.size() >= m_MaxMessages)
		m_ChatMessages.pop_front();

	color.a = 0;

	m_ChatMessage.texts.clear();
	m_ChatMessage.Append(formattedMsgBuffer);
	m_ChatMessage.SetFillColorBack(color);
	for (auto& msg : m_ChatMessages)
		msg.first.Move(0.f, -(GlobChat::m_AppendOffsetY + GlobChat::m_SpaceingY));
	m_ChatMessages.push_back(std::make_pair(m_ChatMessage, m_Clock.getElapsedTime()));

	SetForeground(m_pTextBox->IsActive());
}

void Chat::AppendPlainMessage(sf::Color&& color, const char* message, ...)
{
	char formattedMsgBuffer[512];
	va_list args;
	va_start(args, message);
	vsnprintf(formattedMsgBuffer, sizeof(formattedMsgBuffer), message, args);
	va_end(args);

	if (m_ChatMessages.size() >= m_MaxMessages)
		m_ChatMessages.pop_front();

	color.a = 0;

	m_ChatMessage.texts.clear();
	m_ChatMessage.Append(formattedMsgBuffer);
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
			if (m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Name not found"); return;
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
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "No number specified"); return;
			}
		} break;

		case shared::UserCmd::KILL:
		{
			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Player name not found"); return;
			}
			cmd.iValue = -9999;

		} break;

		case shared::UserCmd::TELEPORT:
		{
			get_next_param();
			if (offset != std::string::npos && m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "First name not found"); return;
			}

			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDRight = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Secound name not found"); return;
			}

		} break;
		case shared::UserCmd::SET_RANK:
		{
			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Bad player name"); return;
			}

			get_next_param();
			if(m_RankIdMap.count(sub))
				cmd.iValue = (int)m_RankIdMap[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Non-existent rank name"); return;
			}

		}break;

		case shared::UserCmd::BAN:
		{
			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Incorrect player name"); return;
			}

			get_next_param();
			if (is_number(sub, true))
			{
				cmd.iValue = std::stoi(sub);
			}
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "No/Negative number specified"); return;
			}
		}break;

		case shared::UserCmd::PRIVATE_MESSAGE:
		{
			get_next_param();
			if (m_NameList.count(sub))
				cmd.uIDLeft = m_NameList[sub];
			else {
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Invalid player name"); return;
			}

			get_next_param();
			if (sub.size() > maxChatMessageLength){
				AppendPlainMessage(sf::Color(122, 95, 48, 255), "Message too big"); return;
			}

			m_pNetwork->SendPrivateChatMessage(sub.c_str(), sub.size(), cmd.uIDLeft);
			return;
		}break;

		case shared::UserCmd::MUTE:
		{
			AudioManager::Get().StopAllSounds();
		}
		break;

		case shared::UserCmd::HELP:
		default: 
		{
			AppendPlainMessage(sf::Color(122, 110, 60, 255), 
								"/msg <player> [message]\n"
								"/add [ammo/health] <player> [amount]\n"
								"/kill <player>\n"
								"/ban <player> [time in sec]\n"
								"/tp <player> <player>\n"
								"/op/deop <player>\n"
								"/setrank <player> [rank]\n"
								"/mute (stops all audio)"
								); return;
		}break;
		}
		
		m_pNetwork->SendChatUserCommand(cmd);
		return;
		
	} while (0);

	AppendPlainMessage(sf::Color(122, 95, 48, 255), "Unknown command");
	

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

void Chat::FindMagicWordInChatMessage(std::string msg)
{
	if (m_SoundNames.find(msg) != m_SoundNames.end()) {
		AudioManager::Get().PlayGameSound((GameSounds)m_SoundNames[msg], 90.f);
		return;
	}

	for (auto& [name, id] : m_SoundNames) {
		if (msg.find(name) != std::string::npos) {
			AudioManager::Get().PlayGameSound((GameSounds)id, 90.f);
			break;
		}
	}
}

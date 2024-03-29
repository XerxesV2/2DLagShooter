#pragma once
#include "Sigleton.hpp"
#include "UI/TextInputBox.hpp"
#include "SharedVariables.hpp"

#include <SFML/Graphics.hpp>
#include <deque>
#include <unordered_map>

namespace GlobChat
{
	inline float m_MessageHight;
	inline sf::Vector2f m_MessagesStartPos = { 0.f, 0.f };
	inline constexpr float m_SpaceingX = 5.f;
	inline constexpr float m_SpaceingY = 3.f;
	//inline constexpr float m_LogDuration = 3.f;
	inline constexpr float m_MessageFadeInTime = 1.f;
	inline float m_AppendOffsetY = 0.f;
}

struct ChatMessage
{
	ChatMessage(sf::Font& font, size_t size) : textTemplate("ABCDE", font, size) { 
		textTemplate.setOrigin(0.f, textTemplate.getGlobalBounds().height + textTemplate.getLocalBounds().top);
	}

	void Move(float x, float y) {
		for (auto& text : texts) {
			text.move(x, y);
		}
	}
	void SetPosition(sf::Vector2f pos) {
		float offs = 0.f;
		for (auto& text : texts) {
			text.setPosition(pos + sf::Vector2f(offs, 0.f));
			offs += text.getGlobalBounds().width + text.getLocalBounds().left + GlobChat::m_SpaceingX;
		}
	}
	void SetPosition(float x, float y) {
		float offs = 0.f;
		for (auto& text : texts) {
			text.setPosition(x + offs, y);
			offs += text.getGlobalBounds().width + text.getLocalBounds().left + GlobChat::m_SpaceingX;
		}
	}
	void SetFillColorBack(sf::Color&& col) {
		if(!texts.empty())
			texts.back().setFillColor(col);
	}
	void SetFillColorBack(sf::Color& col) {
		if (!texts.empty())
			texts.back().setFillColor(col);
	}

	void SetFillColorA(uint8_t a) {
		sf::Color col;
		for (auto& text : texts) {
			col = text.getFillColor();
			col.a = a;
			text.setFillColor(col);
		}
	}

	void Append(const char* str) {
		textTemplate.setString(str);
		
		if (!texts.empty()) {
			textTemplate.move(texts.back().getGlobalBounds().width + texts.back().getLocalBounds().left + GlobChat::m_SpaceingX, 0.f);
		}
		else {
			GlobChat::m_AppendOffsetY = textTemplate.getGlobalBounds().height + textTemplate.getLocalBounds().top;
			textTemplate.setPosition(GlobChat::m_MessagesStartPos.x, GlobChat::m_MessagesStartPos.y - GlobChat::m_AppendOffsetY);
		}
		texts.push_back(textTemplate);
	}

	void Clear()
	{
		textTemplate.setPosition(0.f, 0.f);
		texts.clear();
	}

	std::vector<sf::Text> texts;
	float animPos = 0.f;
	uint8_t aplha = 255;

	sf::Text textTemplate;
};

class GameNetwork;
struct PlayerStruct;
struct ChatMessageData;

class Chat : public Singleton<Chat>, public sf::Drawable
{
public:
	Chat();
	~Chat();

	void Init(sf::RenderWindow* window, PlayerStruct* pLocalPlayer, std::shared_ptr<GameNetwork>& network, float* _pfDeltaTime, std::deque<uint32_t>* keyCodes);
	void Reset();

	void Update();

	const bool IsActive() const { return m_pTextBox->IsActive(); }
	void AppendMessage(PlayerStruct* pPlayer, const ChatMessageData* message);
	void AppendPlainMessage(sf::Color& color, const char* message, ...);
	void AppendPlainMessage(sf::Color&& color, const char* message, ...);

	void AddPlayerNameToPlayerList(uint32_t id, const std::string& name);
	void RemovePlayerFromPlayerList(const std::string& name);

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	void UpdateMessages();
	void HandleUserCommand(std::string& cmd);
	void SetForeground(bool foreground);
	void FindMagicWordInChatMessage(std::string msg);

public:
	sf::RectangleShape m_Background;

private:
	sf::RenderWindow* m_pWindow;
	sf::Font m_Font;
	std::unique_ptr<TextInputBox> m_pTextBox;
	std::shared_ptr<GameNetwork> m_pNetwork;
	PlayerStruct* m_pLocalPlayer = nullptr;

	ChatMessage m_ChatMessage;

	sf::Clock m_Clock;
	std::deque<std::pair<ChatMessage, sf::Time>> m_ChatMessages;

	float* m_pfDeltaTime = nullptr;

	static constexpr float m_ChatSizeX = 600.f;
	static constexpr float m_ChatSizeY = 100.f;
	static constexpr uint32_t m_MaxMessages = 10u;

private:
	std::unordered_map<std::string, uint32_t> m_SoundNames;
	std::unordered_map<std::string, uint32_t> m_NameList;

	enum class AutoCompIndex : unsigned int
	{
		COMMAND,
		NAME
	};

	std::vector<std::string> m_AutoCompleteCommands
	{
		"help",
		"add",
		"ammo",
		"health",
		"tp",
		"kill",
		"ban",
		"op",
		"deop",
		"msg",
		"setrank",
		"op",
		"defa",
		"mute",
	};

	std::unordered_map<std::string, shared::UserCmd> m_Commands
	{
		{"add ammo",	shared::UserCmd::ADD_AMMO},
		{"add health",	shared::UserCmd::ADD_HEALTH},
		{"tp",			shared::UserCmd::TELEPORT},
		{"kill",		shared::UserCmd::KILL},
		{"ban",			shared::UserCmd::BAN},
		{"op",			shared::UserCmd::SET_RANK },
		{"deop",		shared::UserCmd::SET_RANK },
		{"setrank",		shared::UserCmd::SET_RANK},
		{"msg",			shared::UserCmd::PRIVATE_MESSAGE},
		{"mute",		shared::UserCmd::MUTE},
		{"help",		shared::UserCmd::HELP},
	};

	std::unordered_map<shared::PlayerRank, sf::Color> m_RankColorMap
	{
		{ shared::PlayerRank::OWNER, sf::Color(217, 2, 2, 255) },
		{ shared::PlayerRank::OP, sf::Color(194, 240, 10, 255) },
		{ shared::PlayerRank::GAY, sf::Color(255, 10, 218, 255) },
		{ shared::PlayerRank::DEFAULT, sf::Color(97, 198, 201, 255) },
	};

	std::unordered_map<std::string, shared::PlayerRank> m_RankIdMap
	{
		{"op", shared::PlayerRank::OP},
		{"gay", shared::PlayerRank::GAY},
		{ "defa", shared::PlayerRank::DEFAULT },
	};
};


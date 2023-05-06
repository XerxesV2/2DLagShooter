#pragma once
#include "Sigleton.hpp"

#include <SFML/Graphics.hpp>
#include <string_view>
#include <deque>

class LogSystem : public Singleton<LogSystem>, public sf::Drawable
{
public:
	LogSystem();
	~LogSystem();

	void Update();
	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

	void Init(sf::RenderWindow* window);
	void PrintMessage(const char* str, sf::Color color, bool same_line = false);

private:
	sf::RenderWindow* m_pWindow;
	sf::Font m_Font;
	sf::Text m_Text;
	sf::Clock m_Clock;
	std::deque<std::pair<sf::Text, sf::Time>> m_InGameLogs;
	sf::Vector2f m_LogMessagesStartPos;
	float m_LogMessageHight;
};


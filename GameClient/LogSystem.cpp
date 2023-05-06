#include "LogSystem.hpp"

LogSystem::LogSystem()
{
	m_Font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf");
	m_Text.setFont(m_Font);
	m_Text.setCharacterSize(40);
}

LogSystem::~LogSystem()
{
}

void LogSystem::Update()
{
	uint8_t popAmount = 0;
	for (auto& [text, time] : m_InGameLogs)
		if (m_Clock.getElapsedTime().asSeconds() > time.asSeconds() + 5.f)
			++popAmount;

	while (popAmount)
		m_InGameLogs.pop_front(), --popAmount;
}

void LogSystem::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	for (auto& log : m_InGameLogs)
		target.draw(log.first);
}

void LogSystem::Init(sf::RenderWindow* window)
{
	m_pWindow = window;

	m_LogMessagesStartPos = { 0.f,0.f };
	m_Text.setString("ABCDE");
	m_LogMessageHight = m_Text.getGlobalBounds().height;
}

void LogSystem::PrintMessage(const char* str, sf::Color color, bool same_line)
{	
	m_Text.setPosition(m_LogMessagesStartPos);
	m_Text.setString(str);
	m_Text.setFillColor(color);

	if (!same_line)
		for (auto& log : m_InGameLogs)
			log.first.move(0.f, m_LogMessageHight + 10.f);
	else
		m_Text.move(m_InGameLogs.back().first.getGlobalBounds().width + 15.f, 0.f);

	m_InGameLogs.push_back(std::make_pair(m_Text, m_Clock.getElapsedTime()));
}

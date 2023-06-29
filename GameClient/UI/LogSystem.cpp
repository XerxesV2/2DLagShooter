#include "LogSystem.hpp"

LogSystem::LogSystem() : m_Log(m_Font, 40)
{
	m_Font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf");
	m_Log.bg.setFillColor(sf::Color(255, 255, 255, 0));
}

LogSystem::~LogSystem()
{
}

void LogSystem::Update()
{
	uint8_t popAmount = 0;
	for (auto& [log, time] : m_InGameLogs) {
		
		if (m_Clock.getElapsedTime().asSeconds() > time.asSeconds() + m_LogDuration + m_LogFadeTime) {
			++popAmount;
		}
		else if (m_Clock.getElapsedTime().asSeconds() < time.asSeconds() + m_LogDuration - (m_LogDuration - 1.f)) {
			log.animPos = std::min(1.f, log.animPos + *m_pfDeltaTime);
			float easeOut = (1.f - std::powf(1.f - log.animPos, 4.f));

			for (auto& text : log.texts) {
				sf::Color c = text.getFillColor();
				c.a = (sf::Uint8)(255.f * easeOut);
				text.setFillColor(c);
				c = log.bg.getFillColor();
				c.a = (sf::Uint8)(20.f * easeOut);
				log.bg.setFillColor(c);
			}
			log.SetPosition((GlobLog::m_LogMessagesStartPos.x + 30.f) * easeOut, log.texts.back().getPosition().y);
		}
		else if (m_Clock.getElapsedTime().asSeconds() > time.asSeconds() + m_LogDuration) {
			log.animPos = std::max(0.f, log.animPos - *m_pfDeltaTime);
			float easeIn = (std::powf(log.animPos, 4.f));

			for (auto& text : log.texts) {
				sf::Color c = text.getFillColor();
				c.a = (sf::Uint8)(255.f * easeIn);
				text.setFillColor(c);
				c = log.bg.getFillColor();
				c.a = (sf::Uint8)(20.f * easeIn);
				log.bg.setFillColor(c);
			}
			log.SetPosition((GlobLog::m_LogMessagesStartPos.x + 30.f) * easeIn, log.texts.back().getPosition().y);
			
		}
	}
	while (popAmount)
		m_InGameLogs.pop_front(), --popAmount;
}

void LogSystem::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	for (auto& log : m_InGameLogs) {
		target.draw(log.first.bg);
		for (auto& text : log.first.texts)
			target.draw(text);
	}
}

void LogSystem::Init(sf::RenderWindow* window, float* _pfDeltaTime)
{
	m_pWindow = window;
	m_pfDeltaTime = _pfDeltaTime;
	GlobLog::m_LogMessagesStartPos = { 0.f, 50.f };
	GlobLog::m_LogMessageHight = m_Log.textTemplate.getGlobalBounds().height;
}

void LogSystem::PrintMessage(const char* str, sf::Color color, bool same_line)
{	
	color.a = 0;

	if (!same_line) {
		m_Log.texts.clear();
		m_Log.bg.setSize(sf::Vector2f(0.f, 0.f));
		m_Log.Append(str, color);
		m_Log.SetFillColorBack(color);
		for (auto& log : m_InGameLogs)
			log.first.Move(0.f, GlobLog::m_LogMessageHight + m_SpaceingY);
		m_InGameLogs.push_back(std::make_pair(m_Log, m_Clock.getElapsedTime()));
	}
	else {
		m_InGameLogs.back().first.Append(str, color);
		m_InGameLogs.back().first.SetFillColorBack(color);
	}

}

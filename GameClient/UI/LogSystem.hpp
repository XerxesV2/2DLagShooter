#pragma once
#include "../Sigleton.hpp"

#include <SFML/Graphics.hpp>
#include <string_view>
#include <deque>

namespace GlobLog
{
	inline float m_LogMessageHight;
	inline sf::Vector2f m_LogMessagesStartPos = { 0.f, 0.f };
}

class LogSystem : public Singleton<LogSystem>, public sf::Drawable
{
public:
	LogSystem();
	~LogSystem();

	void Update();
	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

	void Init(sf::RenderWindow* window, float* _pfDeltaTime);
	void PrintMessage(const char* str, sf::Color color, bool same_line = false);


private:
	sf::RenderWindow* m_pWindow;
	sf::Font m_Font;

	struct Log
	{
		Log(sf::Font& font, size_t size) : textTemplate("ABCDE", font, size) { }

		void Move(float x, float y) {
			for (auto& text : texts) {
				text.move(x, y);
			}
			bg.move(x, y);
		}
		void SetPosition(sf::Vector2f pos) {
			float offs = 0.f;
			for (auto& text : texts) {
				text.setPosition(pos + sf::Vector2f(offs, 0.f));
				offs = text.getGlobalBounds().width + LogSystem::m_SpaceingX;
			}
			bg.setPosition(pos);
		}
		void SetPosition(float x, float y) {
			float offs = 0.f;
			for (auto& text : texts) {
				text.setPosition(x + offs, y);
				offs = text.getGlobalBounds().width + LogSystem::m_SpaceingX;
			}
			bg.setPosition(x, y );
		}
		void SetFillColorBack(sf::Color col) {
			texts.back().setFillColor(col);
		}

		void Append(const char* str, sf::Color color) {
			textTemplate.setString(str);
			if (!texts.empty()) {
				textTemplate.move(texts.back().getGlobalBounds().width + LogSystem::m_SpaceingX, GlobLog::m_LogMessagesStartPos.y);
				texts.push_back(textTemplate);
			}
			else {
				texts.push_back(textTemplate);
				SetPosition(GlobLog::m_LogMessagesStartPos);
			}
			bg.setSize(sf::Vector2f(bg.getSize().x + textTemplate.getGlobalBounds().width + LogSystem::m_SpaceingX * (texts.size()-1), GlobLog::m_LogMessageHight + LogSystem::m_SpaceingY));
		}

		std::vector<sf::Text> texts;
		sf::RectangleShape bg;
		float animPos;
		
		sf::Text textTemplate;
	};
	
	Log m_Log;

	sf::Clock m_Clock;
	std::deque<std::pair<Log, sf::Time>> m_InGameLogs;
	
	float* m_pfDeltaTime = nullptr;

public:
	static constexpr float m_SpaceingX = 15.f;
	static constexpr float m_SpaceingY = 20.f;
	static constexpr float m_LogDuration = 3.f;
	static constexpr float m_LogFadeTime = 1.f;
};
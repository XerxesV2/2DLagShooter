#pragma once
#include "../Sigleton.hpp"
#include "../PlayerStruct.hpp"
#include "SharedVariables.hpp"

#include <SFML/Graphics.hpp>
#include <deque>
#include <unordered_map>


struct Feed
{
	Feed(sf::Font& font, size_t size) : textTemplate("ABCDE", font, size) {
		textTemplate.setOrigin(0.f, textTemplate.getGlobalBounds().height + textTemplate.getLocalBounds().top);
	}

	void Move(float x, float y) {
		for (auto& text : texts) {
			text.move(x, y);
		}
		animYPos += y;
	}
	void SetPosition(sf::Vector2f pos) {
		float offs = 0.f;
		for (auto& text : texts) {
			text.setPosition(pos + sf::Vector2f(offs, 0.f));
			offs += text.getGlobalBounds().width + text.getLocalBounds().left + m_SpaceingX;
		}
	}
	void SetPosition(float x, float y) {
		float offs = 0.f;
		for (auto& text : texts) {
			text.setPosition(x + offs, y);
			offs += text.getGlobalBounds().width + text.getLocalBounds().left + m_SpaceingX;
		}
	}

	sf::Vector2f GetTextSize() {
		sf::Vector2f size;
		for (auto& text : texts)
			size.x += text.getGlobalBounds().width + text.getLocalBounds().left;
		size.y = texts.back().getGlobalBounds().height + texts.back().getLocalBounds().top;
		return size;
	}

	int GetSize() { return texts.size(); }

	void SetPositionBack(float x, float y) {
		if (!texts.empty())
			texts.back().setPosition(x, y);
	}

	void SetFillColorBack(sf::Color&& col) {
		if (!texts.empty())
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
	
	void SetStyle(sf::Text::Style style) {
		textTemplate.setStyle(style);
	}

	void Append(const char* str) {
		textTemplate.setString(str);

		if (!texts.empty()) {
			textTemplate.move(texts.back().getGlobalBounds().width + texts.back().getLocalBounds().left + m_SpaceingX, 0.f);
		}
		else {
			m_AppendOffsetY = textTemplate.getGlobalBounds().height + textTemplate.getLocalBounds().top;
			textTemplate.setPosition(m_FeedsStartPos.x, m_FeedsStartPos.y + m_AppendOffsetY);
		}
		texts.push_back(textTemplate);
	}

	void Clear()
	{
		textTemplate.setPosition(0.f, 0.f);
		textTemplate.setStyle(sf::Text::Style::Regular);
		texts.clear();
	}

	std::vector<sf::Text> texts;
	float animPos = 0.f;
	float animYPos = 30.f;
	uint8_t aplha = 255;

	sf::Text textTemplate;

public:
	static inline float m_FeedHight;
	static inline sf::Vector2f m_FeedsStartPos = { 0.f, 0.f };
	static inline constexpr float m_SpaceingX = 5.f;
	static inline constexpr float m_SpaceingY = 3.f;
	static inline constexpr float m_FeedDuration = 9.f;
	static inline constexpr float m_FeedFadeInTime = 1.f;
	static inline constexpr float m_FeedFadeOutTime = 3.f;
	static inline float m_AppendOffsetY = 0.f;
};

class Killfeed : public Singleton<Killfeed>, public sf::Drawable
{
public:

	Killfeed();
	~Killfeed();

	void Init(sf::RenderWindow* window, float* _pfDeltaTime);
	void Reset();

	void Update();

	void AppendKillfeed(const PlayerStruct* const pLocalPlayer, const PlayerStruct* const pPerpetrator, const PlayerStruct* const pVictim);

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	void UpdateFeeds();

public:
	sf::RectangleShape m_Background;

private:
	sf::RenderWindow* m_pWindow;
	sf::Font m_Font;

	Feed m_Killfeed;

	sf::Clock m_Clock;
	std::deque<std::pair<Feed, sf::Time>> m_Killfeeds;

	float* m_pfDeltaTime = nullptr;

	static constexpr uint32_t m_MaxKillfeeds = 10u;
};


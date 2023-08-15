#pragma once
#include "../PlayerStruct.hpp"
#include "LogSystem.hpp"
#include "LeaderBoard.hpp"
#include "Killfeed.hpp"

#include <SFML/Graphics.hpp>

class UI : public sf::Drawable
{
public:
	UI(sf::RenderWindow * _pWindow);
	~UI();

	void Init(const PlayerStatsStruct* _pLocalPlayerStat);

	void Update();
	void UpdateName();
	void UpdateFlagScore();

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	sf::RenderWindow* m_pWindow;
	sf::Font m_Font;
	sf::Font m_FontMinecraft;
	sf::Font m_FontPixel;
	sf::Text m_LeftText;
	sf::Text m_RightText;
	sf::Text m_LeftUpText;
	sf::Text m_FlagScoreText;

	const PlayerStatsStruct* m_pLocalPlayerStat;
};


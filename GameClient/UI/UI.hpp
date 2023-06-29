#pragma once
#include "../PlayerStruct.hpp"
#include "LogSystem.hpp"
#include "LeaderBoard.hpp"

#include <SFML/Graphics.hpp>

class UI : public sf::Drawable
{
public:
	UI(sf::RenderWindow * _pWindow);
	~UI();

	void Init(const PlayerStatsStruct* _pLocalPlayerStat);

	void Update();

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	sf::RenderWindow* m_pWindow;
	sf::Font m_Font;
	sf::Text m_LeftText;
	sf::Text m_RightText;
	sf::Text m_LeftUpText;

	const PlayerStatsStruct* m_pLocalPlayerStat;
};


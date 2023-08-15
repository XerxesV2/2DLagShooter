#pragma once
#include "../Sigleton.hpp"
#include "../PlayerStruct.hpp"

#include <SFML/Graphics.hpp>

class LeaderBoard : public Singleton<LeaderBoard>, public sf::Drawable
{
public:
	LeaderBoard();
	~LeaderBoard();

	void Init(sf::Font* font);
	void Reset();

	void UpdateText(PlayerStatsStruct* ps);
	void AddNewItem(PlayerStatsStruct* ps); //maybe change later so if a player exits its stays on the board until the match ends
	void RemoveItem(PlayerStatsStruct* ps);
	void SortItems();

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	struct Item
	{
		PlayerStatsStruct* ps;
		sf::Text text;
	};

	sf::Font* m_pFont;
	sf::Text m_TextBlueprint;

	std::vector<Item> m_Items;

	sf::RectangleShape m_Background;

	static constexpr float m_fTextPadding = 10.f;
	static constexpr float m_fTextHeight = m_fTextPadding + 40.f;
};


#include "LeaderBoard.hpp"
#include "SharedVariables.hpp"
#include <algorithm>

LeaderBoard::LeaderBoard()
{
}

LeaderBoard::~LeaderBoard()
{
}

void LeaderBoard::Init(sf::Font* font)
{
	m_pFont = font;
	m_pFont->loadFromFile("fonts\\Roboto-Medium.ttf");

	m_TextBlueprint.setFont(*m_pFont);
	m_TextBlueprint.setCharacterSize(40);
	m_TextBlueprint.setString("NONE");

	m_Background.setSize(sf::Vector2f(600.f, 100.f));
	m_Background.setPosition(g_CamSizeX - m_Background.getSize().x, 0.f);
	m_Background.setFillColor(sf::Color(255, 255, 255, 50));
}

void LeaderBoard::AddNewItem(PlayerStatsStruct* ps)
{
	m_TextBlueprint.setPosition(m_Background.getGlobalBounds().left + m_fTextPadding, m_Background.getGlobalBounds().top + m_fTextHeight * m_Items.size());
	m_Items.push_back({ps, m_TextBlueprint});
	m_Background.setSize(sf::Vector2f(m_Background.getSize().x, m_fTextHeight * m_Items.size()));
	UpdateText(ps);
}

void LeaderBoard::RemoveItem(PlayerStatsStruct* ps)
{
	if (m_Items.size() == 0) return;
	m_Items.erase(std::remove_if(m_Items.begin(), m_Items.end(), [ps](Item& itm)
		{
			return itm.ps == ps;
		}), m_Items.end());
	m_Background.setSize(sf::Vector2f(m_Background.getSize().x, m_fTextHeight * m_Items.size()));
}

void LeaderBoard::SortItems()
{
	auto scoreCmp = [](Item& item_a, Item& item_b)
	{
		return item_a.ps->score > item_b.ps->score;
	};

	std::sort(m_Items.begin(), m_Items.end(), scoreCmp);
}

void LeaderBoard::UpdateText(PlayerStatsStruct* ps)
{
	for (size_t i = 0; i < m_Items.size(); i++)
	{	
		auto& item = m_Items.at(i);
		item.text.setPosition(m_Background.getGlobalBounds().left, m_Background.getGlobalBounds().top + m_fTextHeight * (float)i);
		//if(item.ps == ps)
			//item.text.setString(std::to_string(i+1) + " " + ps->username + " " + std::to_string(ps->score));
		item.text.setString(std::to_string(i + 1) + ". " + item.ps->username + ": " + std::to_string(item.ps->score));	//oof
	}

	//auto it = std::find(m_Items.begin(), m_Items.end(), ps);
	//it->text.setString(std::to_string(ps->score));
}

void LeaderBoard::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	target.draw(m_Background);
	for (auto& item : m_Items)
	{
		target.draw(item.text);
	}
}

#include "UI.hpp"
#include "SharedVariables.hpp"
#include "../cl_Globals.hpp"
#include <sstream>

UI::UI(sf::RenderWindow* _pWindow)
{
	m_pWindow = _pWindow;

	m_Font.loadFromFile("fonts\\ProggyClean.ttf");
	m_FontMinecraft.loadFromFile("fonts\\Minecraft.ttf");
	m_FontPixel.loadFromFile("fonts\\Pixel.ttf");
	m_LeftText.setFont(m_Font);
	m_LeftUpText.setFont(m_FontPixel);
	m_RightText.setFont(m_Font);
	m_FlagScoreText.setFont(m_FontMinecraft);
	m_LeftText.setCharacterSize(80);
	m_LeftUpText.setCharacterSize(20);
	m_RightText.setCharacterSize(80);
	m_FlagScoreText.setCharacterSize(80);

	m_FlagScoreText.setFillColor(sf::Color(204, 178, 8, 200));

	m_LeftText.setString("HP: 100  AMMO: 100");
	m_LeftUpText.setString("ID: 1000\nPING: 999");
	m_RightText.setString("AAAAAA");
	m_FlagScoreText.setString("100 : 100");

	m_LeftText.setOrigin(0.f, m_LeftText.getGlobalBounds().height + m_LeftText.getGlobalBounds().top);
	m_LeftUpText.setOrigin(0.f, 0.f);
	m_RightText.setOrigin(m_RightText.getGlobalBounds().width, m_RightText.getGlobalBounds().height);
	m_FlagScoreText.setOrigin((m_FlagScoreText.getGlobalBounds().width / 2.f + m_FlagScoreText.getLocalBounds().left), 0.f);

	m_LeftText.setPosition(10.f, g_CamSizeY - 10.f);
	m_LeftUpText.setPosition(5.f, -5.f);
	m_RightText.setPosition(g_CamSizeX - 10.f, g_CamSizeY - 10.f);
	m_FlagScoreText.setPosition(g_CamSizeX / 2.f, 0.f);
}

UI::~UI()
{
}

void UI::Init(const PlayerStatsStruct* _pLocalPlayerStat)
{
	m_pLocalPlayerStat = _pLocalPlayerStat;

	m_RightText.setString(m_pLocalPlayerStat->displayName);

	auto center = sf::Vector2f{ m_RightText.getGlobalBounds().width, m_RightText.getGlobalBounds().height };
	auto localBounds = center + sf::Vector2f{ m_RightText.getLocalBounds().left, m_RightText.getLocalBounds().top };
	//auto rounded = round(localBounds);

	m_RightText.setOrigin(localBounds);

	//m_RightText.setOrigin(m_RightText.getGlobalBounds().width, m_RightText.getGlobalBounds().height * 2.f);
	m_RightText.setPosition(g_CamSizeX - 10.f, g_CamSizeY - 10.f);
}

void UI::Update()
{
	static std::stringstream ss;

	ss << "HP: " << m_pLocalPlayerStat->health;
	ss << "  AMMO: " << m_pLocalPlayerStat->ammo;

	m_LeftText.setString(ss.str());
	ss.str(std::string());

	ss << "ID: " << m_pLocalPlayerStat->ID;
	ss << "\nPING: " << (int)(m_pLocalPlayerStat->ping * 1000.f) << "ms";

	m_LeftUpText.setString(ss.str());
	ss.str(std::string());

	LogSystem::Get().Update();
	Killfeed::Get().Update();
}

void UI::UpdateName()
{
	m_RightText.setString(m_pLocalPlayerStat->displayName);

	auto center = sf::Vector2f{ m_RightText.getGlobalBounds().width, m_RightText.getGlobalBounds().height };
	auto localBounds = center + sf::Vector2f{ m_RightText.getLocalBounds().left, m_RightText.getLocalBounds().top };

	m_RightText.setOrigin(localBounds);
	m_RightText.setPosition(g_CamSizeX - 10.f, g_CamSizeY - 10.f);

}

void UI::UpdateFlagScore()
{
	m_FlagScoreText.setString(std::to_string(g_TeamFlagScore) + " : " + std::to_string(g_EnemyFlagScore));
	m_FlagScoreText.setOrigin((m_FlagScoreText.getGlobalBounds().width / 2.f + m_FlagScoreText.getLocalBounds().left), 0.f);
	m_FlagScoreText.setPosition(g_CamSizeX / 2.f, 0.f);
}

void UI::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	target.draw(m_LeftText);
	target.draw(m_LeftUpText);
	target.draw(m_RightText);
	target.draw(m_FlagScoreText);
	target.draw(LogSystem::Get());
	target.draw(LeaderBoard::Get());
	target.draw(Killfeed::Get());
}

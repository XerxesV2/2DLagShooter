#include "Killfeed.hpp"

Killfeed::Killfeed() : m_Killfeed(m_Font, 30)
{
	m_Font.loadFromFile("fonts\\ASMAN.TTF");
}

Killfeed::~Killfeed()
{
}

void Killfeed::Init(sf::RenderWindow* window, float* _pfDeltaTime)
{
	m_pWindow = window;
	m_pfDeltaTime = _pfDeltaTime;
}

void Killfeed::Reset()
{
	m_Killfeed.Clear();
}

void Killfeed::Update()
{
	UpdateFeeds();
}

void Killfeed::AppendKillfeed(const PlayerStruct* const pLocalPlayer, const PlayerStruct* const pPerpetrator, const PlayerStruct* const pVictim)
{

	m_Killfeed.Clear();

	static sf::Color teamColor;
	static sf::Color hostileTeamColor;
	teamColor = pPerpetrator->stats.team == pLocalPlayer->stats.team ? sf::Color(22, 242, 2, 255) : sf::Color(5, 153, 245, 255);
	hostileTeamColor = pPerpetrator->stats.team != pLocalPlayer->stats.team ? sf::Color(22, 242, 2, 255) : sf::Color(5, 153, 245, 255);

	if(pLocalPlayer == pPerpetrator)
		m_Killfeed.SetStyle(sf::Text::Style::Bold);

	m_Killfeed.Append(pPerpetrator->stats.username.c_str());
	m_Killfeed.SetFillColorBack(teamColor);

	m_Killfeed.Append(" killed ");
	m_Killfeed.SetFillColorBack(sf::Color(200, 200, 200, 255));

	m_Killfeed.Append(pVictim->stats.username.c_str());
	m_Killfeed.SetFillColorBack(hostileTeamColor);

	m_Killfeed.SetPosition(g_CamSizeX - m_Killfeed.GetTextSize().x - Feed::m_SpaceingX * m_Killfeed.GetSize(), Feed::m_FeedsStartPos.y);

	for (auto& log : m_Killfeeds)
		log.first.Move(0.f, (Feed::m_AppendOffsetY + Feed::m_SpaceingY));
	m_Killfeeds.push_back(std::make_pair(m_Killfeed, m_Clock.getElapsedTime()));
}

void Killfeed::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	for (auto& log : m_Killfeeds) {
		for (auto& text : log.first.texts)
			target.draw(text);
	}
}

void Killfeed::UpdateFeeds()
{
	uint8_t popAmount = 0;
	
	for (auto& [log, time] : m_Killfeeds) {

		if (m_Clock.getElapsedTime().asSeconds() > time.asSeconds() + Feed::m_FeedDuration + Feed::m_FeedFadeOutTime) {
			++popAmount;
		}
		else if (m_Clock.getElapsedTime().asSeconds() < time.asSeconds() + Feed::m_FeedFadeInTime) {
			log.animPos = std::min(1.f, log.animPos + *m_pfDeltaTime);
			float easeOut = (1.f - std::powf(1.f - log.animPos, 4.f));

			log.SetFillColorA(log.aplha * easeOut);
			log.SetPosition(log.texts.front().getPosition().x, log.animYPos * easeOut);
		}
		else if (m_Clock.getElapsedTime().asSeconds() > time.asSeconds() + Feed::m_FeedDuration) {
			log.animPos = std::max(0.f, log.animPos - *m_pfDeltaTime * 0.2f);
			float easeIn = (std::powf(log.animPos, 4.f));

			log.SetFillColorA(log.aplha * easeIn);
		}
	}

	while (popAmount)
		m_Killfeeds.pop_front(), --popAmount;
}

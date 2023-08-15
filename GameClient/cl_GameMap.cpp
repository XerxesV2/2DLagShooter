#include "cl_GameMap.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"

GameMap::GameMap()
{
	m_RedFlagTexture.loadFromFile("textures\\flag_red.png");
	m_RedFlagSprite.setTexture(m_RedFlagTexture);
	m_RedFlagSprite.setOrigin(m_RedFlagSprite.getGlobalBounds().width / 2.f, m_RedFlagSprite.getGlobalBounds().height);
	m_RedFlagSprite.setPosition(shared::FlagStandPosLeft.x, shared::FlagStandPosLeft.y);

	m_BlueFlagTexture.loadFromFile("textures\\flag_blue.png");
	m_BlueFlagSprite.setTexture(m_BlueFlagTexture);
	m_BlueFlagSprite.setOrigin(m_BlueFlagSprite.getGlobalBounds().width / 2.f, m_BlueFlagSprite.getGlobalBounds().height);
	m_BlueFlagSprite.setPosition(shared::FlagStandPosRight.x, shared::FlagStandPosRight.y);

	m_RedStandTexture.loadFromFile("textures\\stand_red.png");
	m_RedStandSprite.setTexture(m_RedStandTexture);
	m_RedStandSprite.setOrigin(m_RedStandSprite.getGlobalBounds().getSize() / 2.f);
	m_RedStandSprite.setPosition(shared::FlagStandPosLeft.x, shared::FlagStandPosLeft.y);

	m_BlueStandTexture.loadFromFile("textures\\stand_blue.png");
	m_BlueStandSprite.setTexture(m_BlueStandTexture);
	m_BlueStandSprite.setOrigin(m_BlueStandSprite.getGlobalBounds().getSize() / 2.f);
	m_BlueStandSprite.setPosition(shared::FlagStandPosRight.x, shared::FlagStandPosRight.y);

	sf::RectangleShape objectBlueprint;
	objectBlueprint.setSize(sf::Vector2f(50.f, 600.f));
	objectBlueprint.setPosition(250.f, 200.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(50.f, 600.f));
	objectBlueprint.setPosition(1620.f, 200.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(300.f, 300.f));
	objectBlueprint.setPosition(600.f, 0.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(300.f, 300.f));
	objectBlueprint.setPosition(800.f, 780.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(300.f, 40.f));
	objectBlueprint.setPosition(760.f, 530.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(300.f, 40.f));
	objectBlueprint.setPosition(760.f, 610.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

}

GameMap::~GameMap()
{
}

void GameMap::ArrangePlayerCollision(Vector2f& playerPos)	//bug at two wall intersect
{
	DoPlayerWallCollision(playerPos);
	DoPlayerMapBorderCollsion(playerPos);
}

void GameMap::SetFlagPosition(Vector2f pos, bool red_blue)
{
	sf::Sprite& flagSprite = red_blue ? m_RedFlagSprite : m_BlueFlagSprite;
	flagSprite.setPosition(pos.x, pos.y);
}

Vector2f GameMap::GetRayIntersectionPoint(Vector2f rayStart, Vector2f rayEnd)
{
	Vector2f inters{ 0.f, 0.f };
	Vector2f prevInters{ FLT_MAX, FLT_MAX };
	for (auto& object : m_vObjects) {
		if (Utils::LineRect(rayStart.x, rayStart.y, rayEnd.x, rayEnd.y, object.getPosition().x, object.getPosition().y, object.getLocalBounds().width, object.getLocalBounds().height, inters))
		{
			if (std::sqrt(std::pow(inters.x - rayStart.x, 2) + std::pow(inters.y - rayStart.y, 2)) < std::sqrt(std::pow(prevInters.x - rayStart.x, 2) + std::pow(prevInters.y - rayStart.y, 2)))
				prevInters = inters;
		}
	}
	return prevInters != Vector2f{ FLT_MAX, FLT_MAX } ? prevInters : rayEnd;
}

void GameMap::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	for (auto& object : m_vObjects) {
		target.draw(object);
	}
}

void GameMap::drawFlags(sf::RenderTarget& target) const
{
	target.draw(m_RedStandSprite);
	target.draw(m_BlueStandSprite);
	target.draw(m_RedFlagSprite);
	target.draw(m_BlueFlagSprite);
}

void GameMap::DoPlayerWallCollision(Vector2f& playerPos)
{
	for (auto& object : m_vObjects) {
		Vector2f nearestPoint;
		nearestPoint.x = std::clamp(playerPos.x, object.getPosition().x, object.getPosition().x + object.getSize().x);
		nearestPoint.y = std::clamp(playerPos.y, object.getPosition().y, object.getPosition().y + object.getSize().y);

		Vector2f rayToNearest = nearestPoint - playerPos;
		float rayMag = std::sqrt((rayToNearest.x * rayToNearest.x) + (rayToNearest.y * rayToNearest.y));
		float overlap = g_PlayerRadius - rayMag;

		if (std::isnan(overlap)) overlap = 0.f;

		Vector2f rayNorm = { rayToNearest.x / rayMag, rayToNearest.y / rayMag };

		if (overlap > 0) {
			playerPos = (playerPos - rayNorm * overlap);
		}
	}
}

void GameMap::DoPlayerMapBorderCollsion(Vector2f& playerPos)
{
	if (playerPos.x - g_PlayerRadius < m_MapBB.left) {
		playerPos.x = m_MapBB.left + g_PlayerRadius;
	}
	else if (playerPos.x + g_PlayerRadius > m_MapBB.right) {
		playerPos.x = m_MapBB.right - g_PlayerRadius;
	}
	if (playerPos.y - g_PlayerRadius < m_MapBB.top) {
		playerPos.y = m_MapBB.top + g_PlayerRadius;
	}
	else if (playerPos.y + g_PlayerRadius > m_MapBB.buttom) {
		playerPos.y = m_MapBB.buttom - g_PlayerRadius;
	}
}

#include "GameMap.h"
#include "utils.hpp"
#include "SharedVariables.hpp"

GameMap::GameMap()
{
	sf::RectangleShape objectBlueprint;
	objectBlueprint.setSize(sf::Vector2f(50.f, 200.f));
	objectBlueprint.setPosition(500.f, 300.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(50.f, 200.f));
	objectBlueprint.setPosition(590.f, 300.f);
	objectBlueprint.setFillColor(sf::Color(7, 151, 240, 255));
	m_vObjects.push_back(objectBlueprint);

	objectBlueprint.setSize(sf::Vector2f(50.f, 200.f));
	objectBlueprint.setPosition(500.f, 500.f);
	objectBlueprint.setFillColor(sf::Color(10, 110, 210, 255));
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

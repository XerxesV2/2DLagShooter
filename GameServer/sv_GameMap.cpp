#include "sv_GameMap.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"
#include <algorithm>

GameMap::GameMap()
{
	m_vObjects.push_back({ 500.f, 300.f, 50.f, 200.f});
	m_vObjects.push_back({ 590.f, 300.f, 50.f, 200.f});
	m_vObjects.push_back({ 500.f, 500.f, 50.f, 200.f});
}

GameMap::~GameMap()
{
}

void GameMap::ArrangePlayerCollision(GameState& playerGameState)	//bug at two wall intersect
{
	DoPlayerWallCollision(playerGameState.playerMovementData.v_fPos);
	DoPlayerMapBorderCollsion(playerGameState.playerMovementData.v_fPos);
}

Vector2f GameMap::GetRayIntersectionPoint(Vector2f rayStart, Vector2f rayEnd)
{
	static Vector2f inters{ 0.f, 0.f };
	for (auto& object : m_vObjects) {
		if (Utils::LineRect(rayStart.x, rayStart.y, rayEnd.x, rayEnd.y, object.left, object.top, object.width, object.height, inters))
		{
			printf("ray - wall collision x:%f y:%f\n", inters.x, inters.y);
			return inters;
		}
	}
	return rayEnd;
}

void GameMap::DoPlayerWallCollision(Vector2f& playerPos)
{
	for (auto& object : m_vObjects) {
		Vector2f nearestPoint;
		nearestPoint.x = std::clamp(playerPos.x, object.left, object.left + object.width);
		nearestPoint.y = std::clamp(playerPos.y, object.top, object.top + object.height);

		Vector2f rayToNearest = nearestPoint - playerPos;
		float rayMag = std::sqrt((rayToNearest.x * rayToNearest.x) + (rayToNearest.y * rayToNearest.y));
		float overlap = g_PlayerRadius - rayMag;

		if (std::isnan(overlap)) overlap = 0.f;

		Vector2f rayNorm = { rayToNearest.x / rayMag, rayToNearest.y / rayMag };
		//printf("players x: %f, y: %f\n", playerPos.x, playerPos.v_fPos.y);
		if (std::isnan(rayNorm.x))
		{
			printf("xddd\n");
			return;
		}
		if (overlap > 0) {
			playerPos = playerPos - rayNorm * overlap;
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

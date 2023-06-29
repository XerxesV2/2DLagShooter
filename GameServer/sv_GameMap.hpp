#pragma once
#include "PlayerStats.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"

#include <vector>

class GameMap
{
public:
	GameMap();
	~GameMap();

	//Sets the player position as well
	void ArrangePlayerCollision(GameState& playerGameState);
	//g_RayLength = no collision happened
	Vector2f GetRayIntersectionPoint(Vector2f rayStart, Vector2f rayEnd);

public:
	static constexpr BBox  m_MapBB = { 0.f, 0.f, 1920.f, 1080.f };
private:
	void DoPlayerWallCollision(Vector2f& playerPos);
	void DoPlayerMapBorderCollsion(Vector2f& playerPos);

private:
	std::vector<Rect> m_vObjects;
};


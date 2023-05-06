#pragma once
#include "sv_Player.hpp"

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

private:
	void DoPlayerWallCollision(Vector2f& playerPos);
	void DoPlayerMapBorderCollsion(Vector2f& playerPos);

private:
	std::vector<Rect> m_vObjects;
};


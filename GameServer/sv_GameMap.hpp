#pragma once
#include "PlayerStats.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"

#include <vector>

extern class GameServer;

class GameMap
{
public:
	GameMap();
	~GameMap();

	//Sets the player position as well
	void ArrangePlayerCollision(GameState& playerGameState);
	//g_RayLength = no collision happened
	Vector2f GetRayIntersectionPoint(Vector2f rayStart, Vector2f rayEnd);
	Vector2f GetTeamOneFlagPos() { return m_FlagLeft.pos; }
	Vector2f GetTeamTwoFlagPos() { return m_FlagRight.pos; }
	shared::FlagStates HandleFlagCollision(GameState& playerGameState);
	void DropFlag(GameState& playerGameState, bool lost, GameServer* server);

public:
	static constexpr BBox m_MapBB = { 0.f, 0.f, 1920.f, 1080.f };
	static Vector2f TeamOneSpawnPos;
	static Vector2f TeamTwoSpawnPos;

	static int TeamOneScore;
	static int TeamTwoScore;
private:
	void DoPlayerWallCollision(Vector2f& playerPos);
	void DoPlayerMapBorderCollsion(Vector2f& playerPos);

private:
	struct Flag
	{
		Vector2f pos;
		bool pickedUp;
		bool atItsStartPos = true;
	} m_FlagLeft, m_FlagRight;

private:
	std::vector<Rect> m_vObjects;
};


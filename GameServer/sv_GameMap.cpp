#include "sv_GameMap.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"
#include "Server.hpp"
#include <algorithm>

GameMap::GameMap()
{
	m_FlagLeft.pos = shared::FlagStandPosLeft;
	m_FlagRight.pos = shared::FlagStandPosRight;

	m_vObjects.push_back({ 250.f, 200.f, 50.f, 600.f });
	m_vObjects.push_back({ 1620.f, 200.f, 50.f, 600.f });
	m_vObjects.push_back({ 600.f, 0.f, 300.f, 300.f });
	m_vObjects.push_back({ 800.f, 780.f, 300.f, 300.f });
	m_vObjects.push_back({ 760.f, 530.f, 300.f, 40.f });
	m_vObjects.push_back({ 760.f, 610.f, 300.f, 40.f });
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

shared::FlagStates GameMap::HandleFlagCollision(GameState& playerGameState)
{
	float distance = 0.f;

	int8_t team = playerGameState.playerInfo.team;
	Vector2f playerPos = playerGameState.playerMovementData.v_fPos;
	Vector2f myStandPos = team == 1 ? shared::FlagStandPosLeft : shared::FlagStandPosRight;
	Vector2f enemyStandPos = team == 1 ? shared::FlagStandPosRight : shared::FlagStandPosLeft;	//the opposite team flag stand
	Flag& myFlag = team == 1 ? m_FlagLeft : m_FlagRight;
	Flag& enemyFlag = team == 1 ? m_FlagRight : m_FlagLeft;	//the opposite team flag

	if (!playerGameState.state.b_HasFlag)
	{
		if(enemyFlag.pickedUp) return shared::FlagStates::NONE;

		/* -= Enemy flag anywhere pick up check =- */
		distance = sqrt(pow(playerPos.x - enemyFlag.pos.x, 2) + pow(playerPos.y - enemyFlag.pos.y, 2));
		if (distance < g_PlayerRadius)
		{
			enemyFlag.pickedUp = true;
			enemyFlag.atItsStartPos = false;
			playerGameState.state.b_HasFlag = true;
			return shared::FlagStates::FLAG_STOLEN;
		}
	}
	else
	{
		/* -= Enemy flag capture check =- */
		distance = sqrt(pow(playerPos.x - myStandPos.x, 2) + pow(playerPos.y - myStandPos.y, 2));
		if (myFlag.atItsStartPos && distance < g_PlayerRadius)
		{
			enemyFlag.pickedUp = false;
			enemyFlag.pos = enemyStandPos;
			enemyFlag.atItsStartPos = true;
			playerGameState.state.b_HasFlag = false;
			team == 1 ? ++TeamOneScore : ++TeamTwoScore;
			return shared::FlagStates::FLAG_WIN;
		}
	}

	/* -= My flag return check =- */
	if (myFlag.pickedUp || myFlag.atItsStartPos) return shared::FlagStates::NONE;
	distance = sqrt(pow(playerPos.x - myFlag.pos.x, 2) + pow(playerPos.y - myFlag.pos.y, 2));
	if (distance < g_PlayerRadius)
	{
		myFlag.pos = myStandPos;
		myFlag.atItsStartPos = true;
		return shared::FlagStates::FLAG_RETURNED;
	}

	return shared::FlagStates::NONE;
}

void GameMap::DropFlag(GameState& playerGameState, bool lost, GameServer* server)
{
	playerGameState.state.b_HasFlag = false;
	Flag& flag = playerGameState.playerInfo.team == 1 ? m_FlagRight : m_FlagLeft;
	flag.pos = lost ? playerGameState.playerMovementData.v_fPos : (playerGameState.playerMovementData.v_fPos + Vector2f{25.f, 25.f});
	flag.pickedUp = false;

	MapFlagStatusUpdateData flagData;
	flagData.n_uID = playerGameState.playerInfo.u_id;
	flagData.flagState = lost ? (int8_t)shared::FlagStates::FLAG_LOST : (int8_t)shared::FlagStates::FLAG_DROPPED;
	flagData.v_fPos = flag.pos;
	net::packet<GameMessages> flagStatePkt;
	flagStatePkt << GameMessages::FlagStateUpdate;
	flagStatePkt << flagData;
	server->SendTcpPacketToAll(playerGameState.playerInfo.u_gid, flagStatePkt);
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
			printf("xddd player is clipped!\n");
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

Vector2f GameMap::TeamOneSpawnPos = { 100.f, 540.f };
Vector2f GameMap::TeamTwoSpawnPos = { 1820.f, 540.f };

int GameMap::TeamOneScore = 0;
int GameMap::TeamTwoScore = 0;

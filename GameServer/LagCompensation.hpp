#pragma once
#include <iostream>
#include <deque>
#include <vector>

#include "PacketStruct.hpp"
#include "utils.hpp"
#include "sv_Player.hpp"

inline float g_fDeltaTime = 0.f;
class LagCompensation
{
public:
	LagCompensation(const double* _pCurrentTime);
	~LagCompensation();

	void UpdatePlayerLagRecord(GameState& gameState, double currentTime);
	void Prepare(GameState& playerGameState);
	void Start(const uint32_t playerId, const uint32_t otherPlayerId, GameState& playerGameState, GameState& otherPlayerGameState);
	void End(GameState& otherPlayerGameState);
	void Finish(GameState& playerGameState);
	Vector2f DetermineClientViewInterpolation(Vector2f startPos, const Vector2f targetPosition);

private:
	bool m_bFoundValidRecord = false;
	double m_PacketLatency = 0.0;
	double m_ClientCommandExecutionTime = 0.0;
	double m_ClientInterpolationTime = 0.0;
	const double* m_pCurrentTime = nullptr;

	bool m_bLagComp = true;
	bool m_bLagCompInterP = true;
};


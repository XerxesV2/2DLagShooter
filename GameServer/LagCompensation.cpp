#include "LagCompensation.hpp"
#include "MessageTypes.hpp"
#include "SharedVariables.hpp"

//#define DEBUG

LagCompensation::LagCompensation(const double* _pCurrentTime)
{
	m_pCurrentTime = _pCurrentTime;
}

LagCompensation::~LagCompensation()
{
}

void LagCompensation::UpdatePlayerLagRecord(GameState& gameState, double currentTime)
{
	if (!gameState.playerObjects.records.empty()) {
		gameState.playerObjects.records.erase(
			std::remove_if(gameState.playerObjects.records.begin(), gameState.playerObjects.records.end(),
				[currentTime](const PlayerRecord& record) {
					if (currentTime - record.d_ServerTimePoint >= 1.0) {
						//std::cout << "record outdated...\n"; 
						return true;
					}
					else return false;
				}
			), gameState.playerObjects.records.end()
					);
	}

	if (gameState.playerObjects.records.empty() || gameState.playerObjects.records.back().Pos != gameState.playerMovementData.v_fPos) {
		gameState.playerObjects.records.push_back({ gameState.packetInfo.sv_PacketArriveTime,
													gameState.packetInfo.cl_PacketDispatchTime,
													gameState.playerMovementData.v_fPos });
	}
}

void LagCompensation::Prepare(GameState& playerGameState)
{
	m_PacketLatency = *m_pCurrentTime - playerGameState.packetInfo.cl_PacketDispatchTime;
	m_ClientCommandExecutionTime = *m_pCurrentTime - m_PacketLatency;	//this is unnecessary
	m_ClientInterpolationTime = std::min(playerGameState.playerActionsData.playerActionTime - playerGameState.playerActionsData.serverPacketArriveTime, 1.0 / clientUpdateRate);
}

void LagCompensation::Start(const uint32_t playerId, const uint32_t otherPlayerId, GameState& playerGameState, GameState& otherPlayerGameState)
{
	if (!m_bLagComp) return;

	static PlayerRecord* closestRecord = nullptr;
	static PlayerRecord* behindClosestRecord = nullptr;
#ifdef DEBUG 
	uint32_t debug_recordNum = 0; 
#endif

	for (auto it = otherPlayerGameState.playerObjects.records.rbegin(); it != otherPlayerGameState.playerObjects.records.rend(); ++it) { //Reverse iterators are liars
#ifdef DEBUG 
		++debug_recordNum;
#endif
		//printf("m_ClientCommandExecutionTime %f\n", m_ClientCommandExecutionTime);
		//printf("lastMovementPacketArriveTime: %f\n", (double)playerGameState.playerActionsData.lastMovementPacketArriveTime);
		//printf("record time point: %f\n", it->d_ServerTimePoint);
		if (it->d_ServerTimePoint > m_ClientCommandExecutionTime || (float)it->d_ServerTimePoint > playerGameState.playerActionsData.lastMovementPacketArriveTime) {
			continue;
		}
		else {
			closestRecord = &*it;
			m_bFoundValidRecord = true;
			if(++it != otherPlayerGameState.playerObjects.records.rend())
				behindClosestRecord = &*it;
			break;
		}
	}
	if (m_bFoundValidRecord) { //move back in time
#ifdef DEBUG
		printf("-=					- = -					=-\n");
		//printf("All records: \n");
		//for (int i = 0; i < otherGameState.playerObjects.records.size(); i++)
			//printf("record_%d: %f   y: %f\n", i+1, otherGameState.playerObjects.records.at(i).d_TimePoint, otherGameState.playerObjects.records.at(i).Pos.y);

		printf("Selected record from the back: %d.\nAll records: %d", debug_recordNum, otherPlayerGameState.playerObjects.records.size());
		printf("\nPlayer [%d] Lag compansated %fms\nServerTime: %f\npacketDispatchTime: %f\n(real x:%f, y:%f)\n(compensated x:%f, y:%f)\n\n", otherPlayerId, m_PacketLatency, *m_pCurrentTime, playerGameState.packetInfo.cl_PacketDispatchTime,
			otherPlayerGameState.playerMovementData.v_fPos.x, otherPlayerGameState.playerMovementData.v_fPos.y, closestRecord->Pos.x, closestRecord->Pos.y);
#endif
		if (behindClosestRecord) {
			if (m_bLagCompInterP) {
				otherPlayerGameState.playerMovementData.v_fPos = DetermineClientViewInterpolation(behindClosestRecord->Pos, closestRecord->Pos);
#ifdef DEBUG
				printf("Determined interpolation time is: %fms\n", m_ClientInterpolationTime);
				printf("playerActionTime %f\n", playerGameState.playerActionsData.playerActionTime);
				printf("serverPacketArriveTime %f\n", playerGameState.playerActionsData.serverPacketArriveTime);
#endif
			}else
				otherPlayerGameState.playerMovementData.v_fPos = closestRecord->Pos;
		}
		else {
			otherPlayerGameState.playerMovementData.v_fPos = closestRecord->Pos;
#ifdef DEBUG 
			printf("No record behind this one. Compensation results might be inaccurate!\n");
#endif
		}

		m_bFoundValidRecord = false;
		closestRecord = nullptr;
		behindClosestRecord = nullptr;
	}
	else
		std::cout << "Lag compensation failed (not found valid record)\n";
}

void LagCompensation::End(GameState& otherPlayerGameState)
{
	//check if player died for the respawn 
	if (otherPlayerGameState.health <= 0) {
		otherPlayerGameState.health = 100;
	}
	else {
		otherPlayerGameState.playerMovementData.v_fPos = otherPlayerGameState.playerObjects.records.back().Pos;	//restore player position
	}
}

void LagCompensation::Finish(GameState& playerGameState)
{
	playerGameState.playerObjects.shotRay = false;
}

Vector2f LagCompensation::DetermineClientViewInterpolation(Vector2f startPos, const Vector2f targetPosition)
{
	// Calculate the distance between the current position and the target position
	static float timePeriod = 1.f / clientUpdateRate; // ex. 100ms time period   (1s / server tick rate)
	float dx = targetPosition.x - startPos.x;
	float dy = targetPosition.y - startPos.y;
	float distance = sqrt(dx * dx + dy * dy);
	if (distance <= 0.f) return targetPosition;

	// Calculate the direction vector
	float dirX = dx / distance;
	float dirY = dy / distance;

	// Calculate the required speed based on the distance and time period
	static float requiredSpeed = 0.f;
	requiredSpeed = distance / timePeriod;

	// Calculate the movement vector
	float moveX = dirX * requiredSpeed * m_ClientInterpolationTime;
	float moveY = dirY * requiredSpeed * m_ClientInterpolationTime;
	
	return startPos + Vector2f{ moveX, moveY };
}

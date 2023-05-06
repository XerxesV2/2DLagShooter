#include <NetServer.hpp>
#include <unordered_map>
#define _USE_MATH_DEFINES
#include <math.h>

#include "sv_GameMap.hpp"
#include "LagCompensation.hpp"
#include "PacketStruct.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"

//#define IPIDS

class GameServer : public net::ServerInterface<GameMessages>
{
public:
	GameServer(uint32_t port) : net::ServerInterface<GameMessages>(port)
	{
		m_pLagCompensation = std::make_unique<LagCompensation>(&m_CurrentTime);
		std::cout << "Listening on port: " << port << std::endl;
	}
	~GameServer() {}

protected:
	bool OnClientConnect(std::shared_ptr<net::ServerConnection<GameMessages>> client) override
	{
		std::cout << "Client IP: " << client->GetIP().to_string() << "\n";
		return true;
	}

	void OnClientDisconnect(std::shared_ptr<net::ServerConnection<GameMessages>> client) override {

		std::cout << "Client [" << client->GetID() << "] dissapeared removing..." << std::endl;

		if (m_MapPlayerGameStates.find(client->GetID()) != m_MapPlayerGameStates.end()) {
			m_MapPlayerGameStates.erase(client->GetID());
			net::packet<GameMessages> RMpkt;
			RMpkt << GameMessages::RemovePlayer;
			RMpkt << client->GetID();
			SendPacketToAll(RMpkt, client);

			std::cout << "Sending remove player packet" << std::endl;
		}

	}

	void OnPacketReceived(std::shared_ptr<net::ServerConnection<GameMessages>> client, net::packet<GameMessages>& packet) override 
	{
		while (packet.bytesRead < packet.body.size())
		{
			switch (packet.GetNextSubPacketType())
			{
			case GameMessages::RegisterClient:
			{
				GameState stGameState;
				stGameState.playerMovementData.v_fPos = { 100.f, 100.f };
				stGameState.health = 100;

				stGameState.playerMovementData.n_uID = client->GetID();
#ifdef IPIDS
				m_MapPlayerGameStates.insert_or_assign(client->GetIP().to_v4().to_uint(), stGameState);
				std::cout << "New player ID: " << client->GetIP().to_v4().to_uint() << std::endl;
#else
				m_MapPlayerGameStates.insert_or_assign(client->GetID(), stGameState);
				std::cout << "New player ID: " << client->GetID() << std::endl;
#endif


				net::packet<GameMessages> IDpkt;
				IDpkt << GameMessages::SetClientID;
				IDpkt << client->GetID();
				SendPacket(client, IDpkt);

				net::packet<GameMessages> ADDpktAll;
				ADDpktAll << GameMessages::AddPlayer;
				ADDpktAll << stGameState.playerMovementData;
				SendPacketToAll(ADDpktAll, client);

				for (const auto& player : m_MapPlayerGameStates) {
					net::packet<GameMessages> ADDpktLocal;	//TODO add reset func
					ADDpktLocal << GameMessages::AddPlayer;
					ADDpktLocal << player.second.playerMovementData;
					SendPacket(client, ADDpktLocal);
				}

			} break;

			case GameMessages::UpdatePlayerMovement:
			{
				PlayerMovementData* playerInfo;		//multiple updtates in one packet failes bc everything is in one packet now
				playerInfo = packet.GetSubPacketPtr<PlayerMovementData>();

#ifdef IPIDS
				GameState& playerGameState = m_MapPlayerGameStates[client->GetIP().to_v4().to_uint()];
#else
				GameState& playerGameState = m_MapPlayerGameStates[client->GetID()];
#endif

				playerGameState.packetInfo.sv_PacketArriveTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();;

				if (playerInfo->n_uID != client->GetID()) { printf("bad packet...\n"); break; }
				if (playerInfo->deltaTime > 1.f / 5.f) playerInfo->deltaTime = 1.f / 5.f;	//dont play with 5fps

				/* deltaTime & timeStamp & serverPacketArriveTime not chacked thus can be exploited*/
				if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::move_left) {
					playerGameState.playerMovementData.v_fPos.x -= m_fExpectedPlayerSpeed * playerInfo->deltaTime;
				}
				if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::move_right) {
					playerGameState.playerMovementData.v_fPos.x += m_fExpectedPlayerSpeed * playerInfo->deltaTime;
				}
				if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::move_up) {
					playerGameState.playerMovementData.v_fPos.y -= m_fExpectedPlayerSpeed * playerInfo->deltaTime;
				}
				if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::move_down) {
					playerGameState.playerMovementData.v_fPos.y += m_fExpectedPlayerSpeed * playerInfo->deltaTime;
				}	

				m_Map.ArrangePlayerCollision(playerGameState);	//rip cpu https://www.youtube.com/watch?v=7NFwhd0zsHU 

				if (packet.PeekNextPacketType() == GameMessages::UpdatePlayerMovement) break;	//loop until no more movement data

				//static auto curTime = 0.0;
				//curTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
				////Movement caretaker
				////might prevent some speedy kids
				//if (playerInfo->deltaTime > curTime - playerGameState.packetInfo.sv_PacketArriveTime) {
				//	playerInfo->deltaTime = curTime - playerGameState.packetInfo.sv_PacketArriveTime;
				//	printf("kid might tried to go faster lmao\n");
				//}
				
				playerGameState.playerMovementData.n_uID = client->GetID();
				playerGameState.playerMovementData.n_uSequence = playerInfo->n_uSequence;
				playerGameState.playerMovementData.rotation = playerInfo->rotation;
				playerGameState.playerMovementData.deltaTime = playerInfo->deltaTime;
				playerGameState.playerMovementData.sv_PacketArriveTime = playerGameState.packetInfo.sv_PacketArriveTime;
				playerGameState.playerMovementData.u_PlayerActions = playerInfo->u_PlayerActions;
				playerGameState.packetInfo.cl_PacketDispatchTime = packet.header.sendTime;

				//old
				//if(std::abs(playerInfo.v_fPos.x - m_mapLastPlayerUpdateInfo[playerInfo.n_uID].v_fPos.x) > m_fExpectedPlayerSpeed
				//|| std::abs(playerInfo.v_fPos.y - m_mapLastPlayerUpdateInfo[playerInfo.n_uID].v_fPos.y) > m_fExpectedPlayerSpeed)	//regulate player
				//{
				//	net::packet<GameMessages> lastInfoPkt;
				//	lastInfoPkt.header.id = GameMessages::UpdatePlayer;
				//	//m_mapLastPlayerUpdateInfo[playerInfo.n_uID].v_fPos.x += m_fExpectedPlayerSpeed;	//<--- TODO
				//	lastInfoPkt << m_mapLastPlayerUpdateInfo[playerInfo.n_uID];
				//	std::cout << "Regulate dif: " << std::abs(playerInfo.v_fPos.x - m_mapLastPlayerUpdateInfo[playerInfo.n_uID].v_fPos.x) << std::endl;
				//	SendPacketToAll(lastInfoPkt);
				//	return;
				//}
				//m_mapLastPlayerUpdateInfo[playerInfo.n_uID] = playerInfo;
				////m_mapPlayers[playerInfo.n_uID] = playerInfo;

				//packet << playerInfo;
				////SendPacketToAll(packet, client);
				//SendPacketToAll(packet);
			}break;

			case GameMessages::UpdatePlayerActions:
			{
				PlayerActionsData* playerInfo;
				playerInfo = packet.GetSubPacketPtr<PlayerActionsData>();

#ifdef IPIDS
				GameState& playerGameState = m_MapPlayerGameStates[client->GetIP().to_v4().to_uint()];
#else
				GameState& playerGameState = m_MapPlayerGameStates[client->GetID()];
#endif
				if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) {
					if (playerInfo->playerActionTime - playerGameState.playerObjects.lastShotTime >= m_fExpectedPlayerReloadTime) {
						playerGameState.playerObjects.shotRay = true;	//nah
						playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_ray;
						playerGameState.playerObjects.lastShotTime = playerInfo->playerActionTime;
					}
					else {
						printf("No full auto in building!\n");
						break;
					}
				}

				playerGameState.packetInfo.sv_PacketArriveTime = m_CurrentTime;
				playerGameState.packetInfo.cl_PacketDispatchTime = packet.header.sendTime;
				playerGameState.playerActionsData.n_uID = client->GetID();	//safe
				playerGameState.playerActionsData.serverPacketArriveTime = playerInfo->serverPacketArriveTime;
				playerGameState.playerActionsData.playerActionTime = playerInfo->playerActionTime;
				playerGameState.playerActionsData.lastMovementPacketArriveTime = playerInfo->lastMovementPacketArriveTime;


				if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_bullet) {
					playerGameState.playerObjects.bullets.push_back
					({ playerGameState.playerMovementData.v_fPos + Vector2f(cos(d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f,
						sin(d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f), playerGameState.playerMovementData.rotation });
					playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_bullet;
				}

				
				
			}break;

			default: printf("\nWrong data type!"); break;
			}
		}
		if (packet.bytesRead != packet.body.size())
			printf("\nNot all bytes was read");
	}

	bool OnChecksumMismatch(std::shared_ptr<net::ServerConnection<GameMessages>> client) override {
		return false;	//for test
	}

	void OnChecksumMatch(std::shared_ptr<net::ServerConnection<GameMessages>> client) override {
		std::cout << "Checksum match!" << std::endl;

		net::packet<GameMessages> pkt;
		pkt << GameMessages::ServerAccept;
		client->Send(pkt);
	}

public:
	void UpdateGameState(double currentTime) {
		m_CurrentTime = currentTime;

		for (auto& [playerId, playerGameState] : m_MapPlayerGameStates)
		{
			m_pLagCompensation->UpdatePlayerLagRecord(playerGameState, currentTime);

			if (playerGameState.playerObjects.shotRay) {
				PreCalcShotRay(playerGameState);
				m_pLagCompensation->Prepare(playerGameState);
				for (auto& [otherPlayerId, otherPlayerGameState] : m_MapPlayerGameStates) {
					if (playerId == otherPlayerId) continue; //local player check
					m_pLagCompensation->Start(playerId, otherPlayerId, playerGameState, otherPlayerGameState);
					ProcessPlayerShot(playerGameState, otherPlayerGameState);
					m_pLagCompensation->End(otherPlayerGameState);
				}
				m_pLagCompensation->Finish(playerGameState);

				static net::packet<GameMessages> pkt;
				pkt.header.sendTime = playerGameState.packetInfo.cl_PacketDispatchTime;
				pkt.Clear();
				pkt << GameMessages::UpdatePlayerActions;
				pkt << playerGameState.playerActionsData;
				SendPacketToAll(pkt);
				playerGameState.playerActionsData.u_PlayerActions = 0;	//TODO 
			}

			if (playerGameState.playerMovementData.u_PlayerActions != 0) {
				static net::packet<GameMessages> pkt;
				pkt.header.sendTime = playerGameState.packetInfo.cl_PacketDispatchTime;
				pkt.Clear();
				//pkt.header.sendTime = packet.header.sendTime;
				pkt << GameMessages::UpdatePlayerMovement;
				pkt << playerGameState.playerMovementData;
				SendPacketToAll(pkt);
				playerGameState.playerMovementData.u_PlayerActions = 0;
			}

			//SimulateBullets(playerId, playerGameState);	//breake lag comp for some reason FIXME
		}
	}

	void UnHittableAntiCheat() {

	}

private:
	float d2r(float d) { return (d / 180.0f) * ((float)M_PI); }
	bool CheckCircleCollide(double x1, double y1, float r1, double x2, double y2, float r2) {
		return std::abs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) < (r1 + r2) * (r1 + r2);
	}

	bool TraceRay(GameState& playerGameState, PlayerMovementData& otherPlayerInfo, Vector2f& intersectionPoint) {
		
		return Utils::LineCircleCollide(playerGameState.rayStart.x, playerGameState.rayStart.y,
			playerGameState.rayEnd.x, playerGameState.rayEnd.y,
			otherPlayerInfo.v_fPos.x, otherPlayerInfo.v_fPos.y, m_fPlayerRadius, intersectionPoint);
	}

	void ProcessPlayerShot(GameState& playerGameState, GameState& otherPlayerGameState) {

		static Vector2f intersectionPoint;
		if (TraceRay(playerGameState, otherPlayerGameState.playerMovementData, intersectionPoint)) {
			printf("\n[%d] Player hit ray\n", playerGameState.playerActionsData.n_uID);

			uint32_t damageDealt = CalcDamageAmount(otherPlayerGameState, intersectionPoint);
			printf("Calculated damage: %d\n", damageDealt);
			otherPlayerGameState.health -= damageDealt;
			
			sv_HitregData hitregData;
			hitregData.n_uID = otherPlayerGameState.playerMovementData.n_uID;
			hitregData.u_HitregFlag |= (UINT32)FLAGS_HITREG::player_hit_ray;
			hitregData.u_Damage = damageDealt;
			hitregData.v_fPos = otherPlayerGameState.playerMovementData.v_fPos;

			net::packet<GameMessages> gameStatePkt;
			gameStatePkt << GameMessages::GameStateUpdate;
			gameStatePkt << hitregData;
			SendPacketToAll(gameStatePkt);

			if (otherPlayerGameState.health <= 0)
			{
				printf("[%d] Player died\n", playerGameState.playerActionsData.n_uID);
				otherPlayerGameState.playerMovementData.u_PlayerActions = 0;
				otherPlayerGameState.playerMovementData.v_fPos = { 100.f, 100.f };

				sv_PlayerStateData playerState;
				playerState.n_uID = otherPlayerGameState.playerMovementData.n_uID;
				playerState.u_PlayerStates |= (UINT32)PLAYER_STATE::respawn;
				playerState.v_fPos = { 100.f, 100.f };

				net::packet<GameMessages> gameStatePkt;
				gameStatePkt << GameMessages::PlayerStateUpdate;
				gameStatePkt << playerState;
				SendPacketToAll(gameStatePkt);
			}
			
		}
		
	}

	//determine the damage by how close the shot is to the center
	uint32_t CalcDamageAmount(GameState& hitPlayerGameState, Vector2f rayIntersectionPoint)
	{
		float diviation = std::sqrtf(std::powf(hitPlayerGameState.playerMovementData.v_fPos.x - rayIntersectionPoint.x, 2.f)
								   + std::powf(hitPlayerGameState.playerMovementData.v_fPos.y - rayIntersectionPoint.y, 2.f));

		return std::min((uint32_t)((m_fPlayerRadius - diviation) * (m_fPlayerMaxHealth / m_fPlayerRadius)) + 10u, 100u);
	}

	void PreCalcShotRay(GameState& playerGameState) {
		static Vector2f rayStart{ 0.f, 0.f };
		static Vector2f rayEnd{ 0.f, 0.f };

		playerGameState.rayStart.x = playerGameState.playerMovementData.v_fPos.x;
		playerGameState.rayStart.y = playerGameState.playerMovementData.v_fPos.y;

		playerGameState.rayEnd.x = playerGameState.playerMovementData.v_fPos.x + cos(Utils::d2r(playerGameState.playerMovementData.rotation)) * g_RayLength;
		playerGameState.rayEnd.y = playerGameState.playerMovementData.v_fPos.y + sin(Utils::d2r(playerGameState.playerMovementData.rotation)) * g_RayLength;

		playerGameState.rayEnd = m_Map.GetRayIntersectionPoint(playerGameState.rayStart, playerGameState.rayEnd);
	}

	void SimulateBullets(const uint32_t playerId, GameState& playerGameState) {
		for (size_t i = 0; i < playerGameState.playerObjects.bullets.size(); i++) {
			auto& bullet = playerGameState.playerObjects.bullets.at(i);
			bullet.pos += Vector2f(cos(d2r(bullet.rotation)) * m_fBulletSpeed * g_fDeltaTime, sin(d2r(bullet.rotation)) * m_fBulletSpeed * g_fDeltaTime);
			//std::cout << "bpos x: " << bullet.pos.x << "  y: " << bullet.pos.y << '\n';

			if (bullet.pos.x <= m_MapBB.left || bullet.pos.y <= m_MapBB.top || bullet.pos.x >= m_MapBB.right || bullet.pos.y >= m_MapBB.buttom) {
				playerGameState.playerObjects.bullets.erase(playerGameState.playerObjects.bullets.begin() + i);
				continue;
			}

			for (auto& [otherId, otherPlayerGameState] : m_MapPlayerGameStates)
				if (CheckCircleCollide(otherPlayerGameState.playerMovementData.v_fPos.x, otherPlayerGameState.playerMovementData.v_fPos.y, m_fPlayerRadius, bullet.pos.x, bullet.pos.y, m_fBulletRadius)) {
					std::cout << "collision!\n";

					sv_HitregData hitregData;
					hitregData.n_uID = otherPlayerGameState.playerMovementData.n_uID;
					hitregData.u_HitregFlag |= (UINT32)FLAGS_HITREG::player_hit;
					hitregData.u_Damage = 0u;
					hitregData.v_fPos = otherPlayerGameState.playerMovementData.v_fPos;

					net::packet<GameMessages> gameStatePkt;
					gameStatePkt << GameMessages::GameStateUpdate;
					gameStatePkt << otherPlayerGameState.playerActionsData;
					SendPacketToAll(gameStatePkt);
					playerGameState.playerObjects.bullets.erase(playerGameState.playerObjects.bullets.begin() + i);
					break;
				}
		}
	}

private:
	static constexpr uint32_t GetTickRate() { return m_uTickRate; }

	static constexpr float m_fExpectedPlayerSpeed = 400.f;
	static constexpr float m_fExpectedPlayerReloadTime = 0.5f;
	static constexpr float m_fBulletSpeed = 500.f;
	static constexpr float m_fPlayerRadius = 20.f;
	static constexpr float m_fPlayerMaxHealth = 100.f;
	static constexpr float m_fBulletRadius = 10.f;
	static constexpr BBox  m_MapBB = { 0.f, 0.f, 1920.f, 1080.f };

private:
	/* Lag compensation */
	std::unique_ptr<LagCompensation> m_pLagCompensation;
	/*------------------*/
	GameMap m_Map;

	double m_CurrentTime = 0.0;
	std::unordered_map<uint32_t, GameState> m_MapPlayerGameStates;
	static constexpr uint32_t m_uTickRate = 10;
};

const double targetFrameTime = 1.0 / serverTickRate; // x frames per second
double previousTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
double lagTime = 0.0;
int main() 
{
	GameServer gs(6969);
	std::cout << "Tick rate: " << serverTickRate << std::endl;
	gs.Start();
	while (1)
	{
		double currentTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		double elapsedTime = currentTime - previousTime;
		previousTime = currentTime;

		lagTime += elapsedTime;

		while (lagTime >= targetFrameTime) {
			gs.Update(100, false);
			gs.UpdateGameState(currentTime);

			g_fDeltaTime = elapsedTime;
			//std::cout << "Delta time: " << g_fDeltaTime << " seconds" << std::endl;

			lagTime -= targetFrameTime;
		}
	}

	return EXIT_SUCCESS;
}
#include "sv_Player.hpp"
#include "Server.hpp"

Player::Player(const uint32_t id, GameMap& map, GameServer* const gs) : m_id(id), m_Map(map)
{
	m_Server = gs;
}

Player::~Player()
{
}

void Player::SetClient(std::shared_ptr<net::ServerConnection<GameMessages>>& tcpClient)
{
	m_TcpClient = tcpClient;
}

void Player::Udpate(std::unordered_map<uint32_t, std::shared_ptr<Player>>& mapPayers)
{
	CheckIfPlayerResponding();
	g_pLagCompensation->UpdatePlayerLagRecord(m_PlayerGameState);

	if (m_PlayerGameState.playerObjects.shotRay) {

		PreCalcShotRay(m_PlayerGameState);
		g_pLagCompensation->Prepare(m_PlayerGameState);
		for (auto& [otherPlayerId, otherPlayer] : mapPayers) {
			if (m_PlayerGameState.playerInfo.team == otherPlayer->GetGameState().playerInfo.team) continue; //teammate check
			if (m_id == otherPlayerId) continue; //local player check
			g_pLagCompensation->Start(m_id, otherPlayerId, m_PlayerGameState, otherPlayer->GetGameState());
			ProcessPlayerShot(m_PlayerGameState, otherPlayer->GetGameState());
			//g_pLagCompensation->End(otherPlayer->GetGameState());
		}
		g_pLagCompensation->Finish(m_PlayerGameState);

		static net::udpPacket<GameMessages> pkt;
		pkt.header.sendTime = m_PlayerGameState.packetInfo.cl_PacketDispatchTime;
		pkt.Clear();
		pkt << GameMessages::UpdatePlayerActions;
		pkt << m_PlayerGameState.playerActionsData;
		pkt.AddHeaderToBody();
		m_Server->SendUdpPacketToAll(m_PlayerGameState.playerInfo.u_gid, pkt);
		m_PlayerGameState.playerActionsData.u_PlayerActions = 0;
	}
}

void Player::CheckIfPlayerDied(GameState& playerGameState, GameState& otherPlayerGameState)
{
	if (otherPlayerGameState.stats.health <= 0)
	{
		g_pLagCompensation->End(otherPlayerGameState);	//for the pos update

		printf("[%d] Player died\n", playerGameState.playerActionsData.n_uID);
		playerGameState.stats.ammo += g_KillReward_ammo;
		playerGameState.stats.score += g_KillReward_score;

		if (otherPlayerGameState.state.b_HasFlag)
		{
			m_Server->GetMap().DropFlag(otherPlayerGameState, true, m_Server);
		}

		/*attentione respawn pos set!!!!*/
		otherPlayerGameState.playerMovementData.u_PlayerActions = 0;
		otherPlayerGameState.playerMovementData.v_fPos = { otherPlayerGameState.playerInfo.team == 1 ? GameMap::TeamOneSpawnPos : GameMap::TeamTwoSpawnPos };

		sv_PLayerDiedData playerState;
		playerState.n_uID = otherPlayerGameState.playerMovementData.n_uID;
		playerState.n_uPerpetratorID = playerGameState.playerMovementData.n_uID;
		playerState.v_fRespawnPos = { otherPlayerGameState.playerInfo.team == 1 ? GameMap::TeamOneSpawnPos : GameMap::TeamTwoSpawnPos };

		net::packet<GameMessages> gameStatePkt;
		gameStatePkt << GameMessages::PlayerDied;
		gameStatePkt << playerState;
		m_Server->SendTcpPacketToAll(m_PlayerGameState.playerInfo.u_gid, gameStatePkt);

		otherPlayerGameState.stats.health = 100;
		otherPlayerGameState.stats.ammo = 30;
	}
	else
		g_pLagCompensation->End(otherPlayerGameState);
}

bool Player::CheckCircleCollide(double x1, double y1, float r1, double x2, double y2, float r2) {
	return std::abs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) < (r1 + r2) * (r1 + r2);
}

bool Player::TraceRay(GameState& playerGameState, PlayerMovementData& otherPlayerInfo, Vector2f& intersectionPoint) {

	return Utils::LineCircleCollide(playerGameState.rayStart.x, playerGameState.rayStart.y,
		playerGameState.rayEnd.x, playerGameState.rayEnd.y,
		otherPlayerInfo.v_fPos.x, otherPlayerInfo.v_fPos.y, m_fPlayerRadius, intersectionPoint);
}

void Player::ProcessPlayerShot(GameState& playerGameState, GameState& otherPlayerGameState) {

	static Vector2f intersectionPoint;
	if (TraceRay(playerGameState, otherPlayerGameState.playerMovementData, intersectionPoint)) {
		printf("\n[%d] Player hit ray\n", playerGameState.playerActionsData.n_uID);

		uint32_t damageDealt = CalcDamageAmount(otherPlayerGameState, intersectionPoint);
		printf("Calculated damage: %d\n", damageDealt);
		otherPlayerGameState.stats.health -= damageDealt;

		sv_HitregData hitregData;
		hitregData.n_uID = otherPlayerGameState.playerMovementData.n_uID;
		hitregData.n_uPerpetratorID = playerGameState.playerMovementData.n_uID;
		hitregData.u_HitregFlag |= (UINT32)FLAGS_HITREG::player_hit_ray;
		hitregData.u_Damage = damageDealt;
		hitregData.v_fPos = otherPlayerGameState.playerMovementData.v_fPos;

		net::packet<GameMessages> gameStatePkt;
		gameStatePkt << GameMessages::GameStateUpdate;
		gameStatePkt << hitregData;
		m_Server->SendTcpPacketToAll(m_PlayerGameState.playerInfo.u_gid, gameStatePkt);

		CheckIfPlayerDied(playerGameState, otherPlayerGameState);
	}
	else
		g_pLagCompensation->End(otherPlayerGameState);
}

//determine the damage by how close the shot is to the center
uint32_t Player::CalcDamageAmount(GameState& hitPlayerGameState, Vector2f rayIntersectionPoint)
{
	float diviation = std::sqrtf(std::powf(hitPlayerGameState.playerMovementData.v_fPos.x - rayIntersectionPoint.x, 2.f)
		+ std::powf(hitPlayerGameState.playerMovementData.v_fPos.y - rayIntersectionPoint.y, 2.f));

	return std::min((uint32_t)((m_fPlayerRadius - diviation) * (m_fPlayerMaxHealth / m_fPlayerRadius)) + 10u, 100u);
}

void Player::PreCalcShotRay(GameState& playerGameState) {
	static Vector2f rayStart{ 0.f, 0.f };
	static Vector2f rayEnd{ 0.f, 0.f };

	playerGameState.rayStart.x = playerGameState.playerMovementData.v_fPos.x;
	playerGameState.rayStart.y = playerGameState.playerMovementData.v_fPos.y;

	playerGameState.rayEnd.x = playerGameState.playerMovementData.v_fPos.x + cos(Utils::d2r(playerGameState.playerMovementData.rotation)) * g_RayLength;
	playerGameState.rayEnd.y = playerGameState.playerMovementData.v_fPos.y + sin(Utils::d2r(playerGameState.playerMovementData.rotation)) * g_RayLength;

	playerGameState.rayEnd = m_Map.GetRayIntersectionPoint(playerGameState.rayStart, playerGameState.rayEnd);
}

void Player::CheckIfPlayerResponding()
{
	if (m_PlayerGameState.packetInfo.sv_PacketArriveTime < g_CurrentTime - 1.)
	{
		printf("Player [%d][%s] not responding or afk idgaf, pinging...\n", m_id, m_PlayerGameState.playerInfo.s_Username.c_str());
		net::packet<GameMessages> pingPkt;
		pingPkt << GameMessages::ServerPing;
		m_Server->SendTcpPacket(m_TcpClient, pingPkt);

		m_PlayerGameState.packetInfo.sv_PacketArriveTime += 1.;
	}
}

//void Player::SimulateBullets(const uint32_t playerId, GameState& playerGameState) {
//	for (size_t i = 0; i < playerGameState.playerObjects.bullets.size(); i++) {
//		auto& bullet = playerGameState.playerObjects.bullets.at(i);
//		bullet.pos += Vector2f(cos(d2r(bullet.rotation)) * m_fBulletSpeed * g_fDeltaTime, sin(d2r(bullet.rotation)) * m_fBulletSpeed * g_fDeltaTime);
//		//std::cout << "bpos x: " << bullet.pos.x << "  y: " << bullet.pos.y << '\n';
//
//		if (bullet.pos.x <= m_MapBB.left || bullet.pos.y <= m_MapBB.top || bullet.pos.x >= m_MapBB.right || bullet.pos.y >= m_MapBB.buttom) {
//			playerGameState.playerObjects.bullets.erase(playerGameState.playerObjects.bullets.begin() + i);
//			continue;
//		}
//
//		for (auto& [otherId, otherPlayer] : m_MapPlayers)
//			if (CheckCircleCollide(otherPlayer.GetGameState().playerMovementData.v_fPos.x, otherPlayer.GetGameState().playerMovementData.v_fPos.y, m_fPlayerRadius, bullet.pos.x, bullet.pos.y, m_fBulletRadius)) {
//				std::cout << "collision!\n";
//
//				sv_HitregData hitregData;
//				hitregData.n_uID = otherPlayer.GetGameState().playerMovementData.n_uID;
//				hitregData.u_HitregFlag |= (UINT32)FLAGS_HITREG::player_hit;
//				hitregData.u_Damage = 0u;
//				hitregData.v_fPos = otherPlayer.GetGameState().playerMovementData.v_fPos;
//
//				net::packet<GameMessages> gameStatePkt;
//				gameStatePkt << GameMessages::GameStateUpdate;
//				gameStatePkt << otherPlayer.GetGameState().playerActionsData;
//				SendPacketToAll(gameStatePkt);
//				playerGameState.playerObjects.bullets.erase(playerGameState.playerObjects.bullets.begin() + i);
//				break;
//			}
//	}
//}

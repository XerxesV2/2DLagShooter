#include "Server.hpp"
#define DELIM "<+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+>\n"

GameServer::GameServer(uint16_t tcp_port) : net::ServerInterface<GameMessages>(tcp_port)
{
	std::cout << "TCP Listening on port: " << tcp_port << std::endl;

	printf("Reading database...\n");
	m_Database.LoadFromFile();
	printf("Database loaded!\n");

	m_pChat = std::make_unique<ServerChat>(this);
}
GameServer::~GameServer() 
{

}

void GameServer::ShutDown()
{
	SavePlayerStats();
}

std::shared_ptr<Player> GameServer::GetPlayerById(uint32_t id)
{
	if (m_MapPlayers.count(id))
	{
		return m_MapPlayers[id];
	}

	std::cout << id << " was nullptr <---" << std::endl;
	return nullptr;
}

bool GameServer::OnClientConnect(std::shared_ptr<net::ServerConnection<GameMessages>> tcpClient, std::shared_ptr<net::ServerUdpConnection<GameMessages>> udpClient)
{
	std::cout << "Client IP: " << tcpClient->GetIP().to_string() << "\n";

	return true;
}

void GameServer::OnClientDisconnect(const std::shared_ptr<net::ServerConnection<GameMessages>>& client)  {

	std::cout << "Client [" << client->GetID() << "] dissapeared removing..." << std::endl;

	if (m_MapPlayers.count(client->GetID()) != 0)
		m_DisconnectedPlayers.insert(client->GetID());
	else
		RemoveConnection(client);
}

void GameServer::OnUdpPacketReceived(std::shared_ptr<net::ServerUdpConnection<GameMessages>> client, net::udpPacket<GameMessages>& packet)
{
	static float prevSendTime = 0.f;
	if (packet.header.sendTime < prevSendTime) {
		//printf("UDP packet out of order! sendTime: %d\n", packet.header.sendTime);
	}
	prevSendTime = packet.header.sendTime;

	if (client == nullptr) { printf("[ERROR] Client was nullptr\n"); return; }
	while (packet.bytesRead < packet.body.size())
	{
		GameMessages gameMessage = packet.GetNextSubPacketType();
		switch (gameMessage)
		{

		case GameMessages::UpdatePlayerMovement:
		{
			PlayerMovementData* playerInfo;		//multiple updtates in one packet failes bc everything is in one packet now
			playerInfo = packet.GetSubPacketPtr<PlayerMovementData>();
			if (playerInfo == nullptr) {
				printf("[UpdatePlayerMovement] playerInfo was nullptr...\n");
				break;
			}

			if (m_MapPlayers.find(client->GetID()) == m_MapPlayers.end()) {
				printf("[UpdatePlayerMovement] cant find player in m_MapPlayers\n");
				break;
			}

#ifdef IPIDS
			GameState& playerGameState = m_MapPlayers[client->GetIP().to_v4().to_uint()];
#else
			GameState& playerGameState = m_MapPlayers[client->GetID()]->GetGameState();
#endif

			playerGameState.packetInfo.sv_PacketArriveTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();;

			if (m_MapPlayers.find(client->GetID()) == m_MapPlayers.end()) {
				printf("[UpdatePlayerMovement] player id[%d] not found. This occurs after ban.\n", client->GetID());
				break;
			}

			if (playerInfo->n_uID != client->GetID()) { printf("bad packet... ID missmatch, client ID: %d\n", client->GetID()); break; }
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
			//	SendTcpPacketToAll(lastInfoPkt);
			//	return;
			//}
			//m_mapLastPlayerUpdateInfo[playerInfo.n_uID] = playerInfo;
			////m_mapPlayers[playerInfo.n_uID] = playerInfo;

			//packet << playerInfo;
			////SendTcpPacketToAll(packet, client);
			//SendTcpPacketToAll(packet);
		}break;
		case GameMessages::UpdatePlayerActions:
		{
			PlayerActionsData* playerInfo;
			playerInfo = packet.GetSubPacketPtr<PlayerActionsData>();

			if (playerInfo == nullptr) {
				printf("[UpdatePlayerMovement] playerInfo was nullptr...\n");
				break;
			}

			if (m_MapPlayers.find(client->GetID()) == m_MapPlayers.end()) {
				printf("[UpdatePlayerMovement] player id[%d] not found. This occurs after ban.\n", client->GetID());
				break;
			}

#ifdef IPIDS
			GameState& playerGameState = m_MapPlayers[client->GetIP().to_v4().to_uint()];
#else
			GameState& playerGameState = m_MapPlayers[client->GetID()]->GetGameState();
#endif
			if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) {
				if (playerGameState.stats.ammo > 0 && playerInfo->playerActionTime - playerGameState.playerObjects.lastShotTime >= g_fPlayerReloadTime) {
					playerGameState.playerObjects.shotRay = true;	//nah
					playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_ray;
					playerGameState.playerObjects.lastShotTime = playerInfo->playerActionTime;
					--playerGameState.stats.ammo;
				}
				else {
					playerGameState.stats.ammo <= 0 ?
						printf("Ammo count missmatch...\n") :
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
			playerGameState.playerActionsData.v_fPos = playerInfo->v_fPos;
			playerGameState.playerActionsData.rotation = playerInfo->rotation;


			if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_bullet) {
				playerGameState.playerObjects.bullets.push_back
				({ playerGameState.playerMovementData.v_fPos + Vector2f(cos(Utils::d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f,
					sin(Utils::d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f), playerGameState.playerMovementData.rotation });
				playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_bullet;
			}



		}break;

		default: printf("Wrong udp data type! [%d] size(%d)\n", (int)gameMessage, packet.body.size()); 
			packet.Clear();
			break;

		}
	}
	if (packet.bytesRead != packet.body.size()) {
		packet.Clear();
		printf("[WARNING] UDP Not all bytes was read\n");
	}
}

void GameServer::OnTcpPacketReceived(std::shared_ptr<net::ServerConnection<GameMessages>> client, net::packet<GameMessages>& packet)
{
	while (packet.bytesRead < packet.body.size())
	{
		switch (packet.GetNextSubPacketType())
		{
		case GameMessages::RegisterClient:
		{
			if (!m_MapPlayers.count(client->GetID())) {
				printf("-= RegisterClient Client ID not found SUS =-\n");
				break;
			}
			std::shared_ptr<Player>& player = m_MapPlayers[client->GetID()];

			net::packet<GameMessages> IDpkt;
			IDpkt << GameMessages::SetClientID;
			IDpkt << client->GetID();
			SendTcpPacket(client, IDpkt);

			MoveConnectionToGroup(client, (uint32_t)net::ConnectionGroup::GAME);
			player->GetGameState().playerInfo.u_gid = client->GetGroupID();

			player->GetGameState().playerAddData.n_uID = client->GetID();
			player->GetGameState().playerAddData.v_fPos = player->GetGameState().playerMovementData.v_fPos;
			player->GetGameState().playerAddData.score = player->GetGameState().stats.score;
			player->GetGameState().playerAddData.rank = (uint8_t)player->GetGameState().playerInfo.rank;
			player->GetGameState().playerAddData.b_WasInGame = false;
			player->GetGameState().playerAddData.team = player->GetGameState().playerInfo.team;
			player->GetGameState().playerAddData.b_HasFlag = player->GetGameState().state.b_HasFlag;
			std::memcpy(player->GetGameState().playerAddData.sz_Unsername, player->GetGameState().playerInfo.s_Username.c_str(), player->GetGameState().playerInfo.s_Username.size());

			net::packet<GameMessages> ADDpktAll;
			ADDpktAll << GameMessages::AddPlayer;
			ADDpktAll << player->GetGameState().playerAddData;
			SendTcpPacketToAll(client->GetGroupID(), ADDpktAll, client);

			for (const auto& player : m_MapPlayers) {
				net::packet<GameMessages> ADDpktLocal;
				ADDpktLocal << GameMessages::AddPlayer;
				player.second->GetGameState().playerAddData.v_fPos = player.second->GetGameState().playerMovementData.v_fPos;
				player.second->GetGameState().playerAddData.score = player.second->GetGameState().stats.score;
				player.second->GetGameState().playerAddData.rank = (uint8_t)player.second->GetGameState().playerInfo.rank;
				player.second->GetGameState().playerAddData.b_WasInGame = true;
				player.second->GetGameState().playerAddData.team = player.second->GetGameState().playerInfo.team;
				player.second->GetGameState().playerAddData.b_HasFlag = player.second->GetGameState().state.b_HasFlag;
				std::memcpy(player.second->GetGameState().playerAddData.sz_Unsername, player.second->GetGameState().playerInfo.s_Username.c_str(), player.second->GetGameState().playerInfo.s_Username.size());
				ADDpktLocal << player.second->GetGameState().playerAddData;
				SendTcpPacket(client, ADDpktLocal);
			}

			WorldData worldData;
			worldData.i_TeamOneScore = GameMap::TeamOneScore;
			worldData.i_TeamTwoScore = GameMap::TeamTwoScore;
			worldData.v_fTeamOneFlagPos = m_Map.GetTeamOneFlagPos();
			worldData.v_fTeamTwoFlagPos = m_Map.GetTeamTwoFlagPos();
			net::packet<GameMessages> WorldDataPkt;
			WorldDataPkt << GameMessages::WorldState;
			WorldDataPkt << worldData;
			SendTcpPacketToAll(client->GetGroupID(), WorldDataPkt);

			printf("<+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+><+>\n");

		} break;

		case GameMessages::Disconnect:
		{
			printf("Client [%d] disconnected gracefully\n", client->GetID());
			OnClientDisconnect(client);
			//client->Disconnect();
		}break;

		case GameMessages::AutoLogin:
		{
			shared::LoginInformation* loginInfo;
			loginInfo = packet.GetSubPacketPtr<shared::LoginInformation>();

			bool exists = m_Database.IsPlayerExists(*loginInfo);
			net::packet<GameMessages> loginStatusPkt;
			loginStatusPkt << (exists ? GameMessages::AutoLoginSuccess : GameMessages::AutoLoginFail);

			if (!exists) { SendTcpPacket(client, loginStatusPkt); printf("[ERROR] No info found but AutoLogin\n" DELIM); break; }

			if (m_Database.IsPlayerOnline(loginInfo->sz_hwid)) {
				if (m_Database.GetPlayerRank(loginInfo->sz_hwid) == shared::PlayerRank::OWNER) {
					printf("Player is already logged in but OWNER\n");
				}
				else {
					printf("[ERROR] Player is already logged in\n" DELIM);
					net::packet<GameMessages> mysticPkt;
					mysticPkt << GameMessages::MysteriousError;
					SendTcpPacket(client, mysticPkt);
					break;
				}
			}

			if (AddPlayer(client, loginInfo)) break;

			SendTcpPacket(client, loginStatusPkt);
		}break;

		case GameMessages::Login:
		{
			if (m_MapPlayers.count(client->GetID())) {
				printf("[LOGIN FAIL] Player with ID: %d already exists!\n" DELIM, client->GetID());
				net::packet<GameMessages> loginRefPkt;
				loginRefPkt << GameMessages::LoginRefuse;
				SendTcpPacket(client, loginRefPkt);
				break;
			}

			shared::LoginInformation* loginInfo;
			loginInfo = packet.GetSubPacketPtr<shared::LoginInformation>();
			if (loginInfo == nullptr) {
				printf("[LOGIN FAIL] loginInfo was nullptr\n" DELIM);
			}

			if (m_Database.IsPlayerExists(loginInfo->sz_hwid)) {
				printf("[LOGIN FAIL] Player is already logged in with another name\n" DELIM);
				net::packet<GameMessages> loginRefPkt;
				loginRefPkt << GameMessages::MysteriousError;
				SendTcpPacket(client, loginRefPkt);
				break;
			}

			if (strlen(loginInfo->sz_unsername) < 4) {
				printf("[LOGIN FAIL] Login name [%s] is too short\n", loginInfo->sz_unsername);
				net::packet<GameMessages> loginTakenNamePkt;
				loginTakenNamePkt << GameMessages::LoginRefuse;
				SendTcpPacket(client, loginTakenNamePkt);
				break;
			}

			if (m_Database.IsPlayerNameExists(loginInfo->sz_unsername)) {
				printf("[LOGIN FAIL] Login name [%s] is occupied\n", loginInfo->sz_unsername);
				net::packet<GameMessages> loginTakenNamePkt;
				loginTakenNamePkt << GameMessages::LoginNameTaken;
				SendTcpPacket(client, loginTakenNamePkt);
				break;
			}

			bool login_failed = false;
			for (auto ch : loginInfo->sz_unsername) {
				if (ch == 0) break;
				if (ch < 48 || ch > 126) {
					net::packet<GameMessages> loginRefPkt;
					loginRefPkt << GameMessages::LoginRefuse;
					SendTcpPacket(client, loginRefPkt);
					login_failed = true;
					printf("[LOGIN FAIL] Connection refused BAD USERNAME\n" DELIM);
					break;
				}
			}

			if (login_failed) break;

			if (AddPlayer(client, loginInfo)) break;

			net::packet<GameMessages> loginAccPkt;
			loginAccPkt << GameMessages::LoginAccept;
			SendTcpPacket(client, loginAccPkt);

		} break;

		case GameMessages::FlagStateUpdate:
		{
			MapFlagStatusUpdateData* flagData;
			flagData = packet.GetSubPacketPtr<MapFlagStatusUpdateData>();
			if (m_MapPlayers.find(flagData->n_uID) != m_MapPlayers.end()) {
				if (!m_MapPlayers[flagData->n_uID]->GetGameState().state.b_HasFlag) break;
				m_Map.DropFlag(m_MapPlayers[flagData->n_uID]->GetGameState(), false, this);
			}
			break;
		}

		case GameMessages::ChatMessage:
		{
			if (packet.body.size() != sizeof(GameMessages::ChatMessage) + sizeof(ChatMessageData)) {
				printf("\nDefferent ChatMessage size!!!");
				break;
			}

			m_pChat->HandleIncomingMessage(client, packet.GetSubPacketPtr<ChatMessageData>());
		}break;

		case GameMessages::ChatCommand:
		{
			m_pChat->HandleUserCommand(client, packet.GetSubPacketPtr<shared::UserCommand>());
		}break;

		case GameMessages::ServerPing: break;
		default: printf("\nWrong tcp data type!"); break;
		}
	}
	if (packet.bytesRead != packet.body.size())
		printf("\nTCP Not all bytes was read");
}

bool GameServer::OnChecksumMismatch(std::shared_ptr<net::ServerConnection<GameMessages>> client) {
	return false;	//for test
}

void GameServer::OnChecksumMatch(std::shared_ptr<net::ServerConnection<GameMessages>> client) {
	std::cout << "Checksum match!" << std::endl;

	net::packet<GameMessages> pkt;
	pkt << GameMessages::ServerAccept;
	client->Send(pkt);
}

void GameServer::SavePlayerStats()
{
	printf("Saveing player infos...\n");
	for(const auto& [id, player] : m_MapPlayers)
		m_Database.UpdatePlayerData(player->GetGameState());

	m_Database.SaveToFile();
	printf("Player infos saved!\n");
}

bool GameServer::AddPlayer(std::shared_ptr<net::ServerConnection<GameMessages>> client, shared::LoginInformation* loginInfo)
{
	if (m_Database.IsPlayerBanned(*loginInfo)) {
		printf("Banned player [%d] tried to log in\n", client->GetID());
		net::packet<GameMessages> banPkt;
		banPkt << GameMessages::BannedClient;
		banPkt << m_Database.GetBanTime(loginInfo->sz_hwid);
		SendTcpPacket(client, banPkt);
		return 1;
	}

	std::shared_ptr player = std::make_shared<Player>(client->GetID(), m_Map, this);
	player->SetClient(client);
	player->GetGameState().playerInfo.rank = shared::PlayerRank::DEFAULT;
	player->GetGameState().playerInfo.s_Username.assign(loginInfo->sz_unsername);
	player->GetGameState().playerInfo.s_Hwid.assign(loginInfo->sz_hwid);
	player->GetGameState().playerInfo.u_id = client->GetID();
	player->GetGameState().playerInfo.u_gid = client->GetGroupID();
	player->GetGameState().playerInfo.rank = shared::PlayerRank::DEFAULT;
	player->GetGameState().stats.health = 100;
	player->GetGameState().stats.ammo = 30;
	player->GetGameState().stats.score = 0;
	player->GetGameState().playerMovementData.n_uID = client->GetID();
	player->GetGameState().packetInfo.sv_PacketArriveTime = m_CurrentTime + 3.; //3 sec for the respond check

	if (m_Database.GetPlayerStats(player->GetGameState())) {
		m_Database.AddNewPlayer(client->GetID(), *loginInfo);
	}

	std::memcpy(player->GetGameState().playerAddData.sz_Unsername, player->GetGameState().playerInfo.s_Username.c_str(), player->GetGameState().playerInfo.s_Username.size());
	printf("Player logged in with the username: %s\n", player->GetGameState().playerInfo.s_Username.c_str());

	MatchmakerHandlePlayer(*player, true);

#ifdef IPIDS
	m_MapPlayers.insert_or_assign(client->GetIP().to_v4().to_uint(), stGameState);
	std::cout << "New player ID: " << client->GetIP().to_v4().to_uint() << std::endl;
#else
	m_MapPlayers.insert_or_assign(client->GetID(), player);
	std::cout << "New player ID: " << client->GetID() << std::endl;
#endif

	return 0;
}

void GameServer::MatchmakerHandlePlayer(Player& player, bool join_or_left)
{
	static uint32_t leftTeamQuantity = 0;
	static uint32_t rightTeamQuantity = 0;

	if (!join_or_left)
	{
		if (player.GetGameState().playerInfo.team == 1)
			--leftTeamQuantity;
		else
			--rightTeamQuantity;
		return;
	}

	if (leftTeamQuantity > rightTeamQuantity) {
		++rightTeamQuantity;	//rightTeamQuantity += (1 + (-2 * (int)!join_or_left)); //:ccc
		player.GetGameState().playerInfo.team = 2;
		printf("[%s] is added to the RIGHT team\n", player.GetGameState().playerInfo.s_Username.c_str());
	}
	else {
		++leftTeamQuantity;
		player.GetGameState().playerInfo.team = 1;
		printf("[%s] is added to the LEFT team\n", player.GetGameState().playerInfo.s_Username.c_str());
	}
	player.GetGameState().playerMovementData.v_fPos = player.GetGameState().playerInfo.team == 1 ? GameMap::TeamOneSpawnPos : GameMap::TeamTwoSpawnPos;
}

void GameServer::UpdateGameState(double currentTime) {
	m_CurrentTime = currentTime;
	g_CurrentTime = currentTime;

	RemoveDisconnectedPlayers();

	for (auto& [playerId, player] : m_MapPlayers)
	{
		player->Udpate(m_MapPlayers);

		if (player->GetGameState().playerMovementData.u_PlayerActions != 0) {
			static net::udpPacket<GameMessages> pkt;
			pkt.header.sendTime = player->GetGameState().packetInfo.cl_PacketDispatchTime;
			pkt.Clear();
			//pkt.header.sendTime = packet.header.sendTime;
			pkt << GameMessages::UpdatePlayerMovement;
			pkt << player->GetGameState().playerMovementData;
			pkt.AddHeaderToBody();
			SendUdpPacketToAll(player->GetGameState().playerInfo.u_gid, pkt);
			player->GetGameState().playerMovementData.u_PlayerActions = 0;
		}

		shared::FlagStates flagState = m_Map.HandleFlagCollision(player->GetGameState());
		if (flagState != shared::FlagStates::NONE) {
			MapFlagStatusUpdateData flagData;
			flagData.n_uID = player->GetGameState().playerInfo.u_id;
			flagData.flagState = (int8_t)flagState;
			flagData.v_fPos = player->GetGameState().playerMovementData.v_fPos;
			net::packet<GameMessages> flagStatePkt;
			flagStatePkt << GameMessages::FlagStateUpdate;
			flagStatePkt << flagData;
			SendTcpPacketToAll(player->GetGameState().playerInfo.u_gid, flagStatePkt);
		}

		//SimulateBullets(playerId, playerGameState);	//breake lag comp for some reason FIXME
	}
}

void GameServer::RemoveDisconnectedPlayers() {
	if (m_DisconnectedPlayers.empty()) return;

	for (auto& playerID : m_DisconnectedPlayers) {

		if (m_MapPlayers[playerID]->GetGameState().state.b_HasFlag)
			m_Map.DropFlag(m_MapPlayers[playerID]->GetGameState(), true, this);

		MatchmakerHandlePlayer(*m_MapPlayers[playerID], false);
		m_Database.UpdatePlayerData(m_MapPlayers[playerID]->GetGameState());
		m_Database.PlayerLeft(m_MapPlayers[playerID]->GetGameState().playerInfo.s_Hwid);

		net::packet<GameMessages> RMpkt;
		RMpkt << GameMessages::RemovePlayer;
		RMpkt << playerID;
		SendTcpPacketToAll(m_MapPlayers[playerID]->GetGameState().playerInfo.u_gid, RMpkt, m_MapPlayers[playerID]->GetTcpClient());
		RemoveConnection(m_MapPlayers[playerID]->GetTcpClient());
		m_MapPlayers.erase(playerID);

	}
	// printf("Player ID to remove not found... thats... a problem!\n");
	m_DisconnectedPlayers.clear();
}

void GameServer::BanPlayer(uint32_t id, std::string& hwid, long long banTimeInSec)
{
	m_Database.BanPlayer(hwid, banTimeInSec);

	net::packet<GameMessages> discPkt;
	discPkt << GameMessages::Disconnect;
	SendTcpPacket(GetPlayerById(id)->GetTcpClient(), discPkt);

	OnClientDisconnect(GetPlayerById(id)->GetTcpClient());
	RemoveConnection(GetPlayerById(id)->GetTcpClient());
	RemoveDisconnectedPlayers();
}

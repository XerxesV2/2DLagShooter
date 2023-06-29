#include "Server.hpp"


GameServer::GameServer(uint32_t port) : net::ServerInterface<GameMessages>(port, 7777)
{
	std::cout << "TCP Listening on port: " << port << std::endl;
	std::cout << "UDP Listening on port: " << 7777 << std::endl;

	m_pChat = std::make_unique<ServerChat>(this);
}
GameServer::~GameServer() {}

std::shared_ptr<Player> GameServer::GetPlayerById(uint32_t id)
{
	if (m_MapPlayers.count(id))
	{
		return m_MapPlayers[id];
	}

	return nullptr;
}

bool GameServer::OnClientConnect(std::shared_ptr<net::ServerConnection<GameMessages>> tcpClient, std::shared_ptr<net::ServerUdpConnection<GameMessages>> udpClient)
{
	std::cout << "Client IP: " << tcpClient->GetIP().to_string() << "\n";

	return true;
}

void GameServer::OnClientDisconnect(std::shared_ptr<net::ServerConnection<GameMessages>> client)  {

	std::cout << "Client [" << client->GetID() << "] dissapeared removing..." << std::endl;

	m_DisconnectedPlayers.push_back(client->GetID());

}

void GameServer::OnUdpPacketReceived(std::shared_ptr<net::ServerUdpConnection<GameMessages>> client, net::packet<GameMessages>& packet)
{
	while (packet.bytesRead < packet.body.size())
	{
		switch (packet.GetNextSubPacketType())
		{

		case GameMessages::UpdatePlayerMovement:
		{
			PlayerMovementData* playerInfo;		//multiple updtates in one packet failes bc everything is in one packet now
			playerInfo = packet.GetSubPacketPtr<PlayerMovementData>();

#ifdef IPIDS
			GameState& playerGameState = m_MapPlayers[client->GetIP().to_v4().to_uint()];
#else
			GameState& playerGameState = m_MapPlayers[client->GetID()]->GetGameState();
#endif

			playerGameState.packetInfo.sv_PacketArriveTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();;

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

#ifdef IPIDS
			GameState& playerGameState = m_MapPlayers[client->GetIP().to_v4().to_uint()];
#else
			GameState& playerGameState = m_MapPlayers[client->GetID()]->GetGameState();
#endif
			if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) {
				if (playerGameState.ammo > 0 && playerInfo->playerActionTime - playerGameState.playerObjects.lastShotTime >= m_fExpectedPlayerReloadTime) {
					playerGameState.playerObjects.shotRay = true;	//nah
					playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_ray;
					playerGameState.playerObjects.lastShotTime = playerInfo->playerActionTime;
					--playerGameState.ammo;
				}
				else {
					playerGameState.ammo <= 0 ?
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


			if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_bullet) {
				playerGameState.playerObjects.bullets.push_back
				({ playerGameState.playerMovementData.v_fPos + Vector2f(cos(Utils::d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f,
					sin(Utils::d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f), playerGameState.playerMovementData.rotation });
				playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_bullet;
			}



		}break;

		default: printf("\nWrong udp data type!"); break;

		}
	}
	if (packet.bytesRead != packet.body.size())
		printf("\nUDP Not all bytes was read");
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
				printf("RegisterClient Client ID not found SUS\n");
				break;
			}
			std::shared_ptr<Player>& player = m_MapPlayers[client->GetID()];

			net::packet<GameMessages> IDpkt;
			IDpkt << GameMessages::SetClientID;
			IDpkt << client->GetID();
			SendTcpPacket(client, IDpkt);

			net::packet<GameMessages> ADDpktAll;
			ADDpktAll << GameMessages::AddPlayer;
			player->GetGameState().playerAddData.score = player->GetGameState().score;
			ADDpktAll << player->GetGameState().playerAddData;
			SendTcpPacketToAll(ADDpktAll, client);

			for (const auto& player : m_MapPlayers) {
				net::packet<GameMessages> ADDpktLocal;
				ADDpktLocal << GameMessages::AddPlayer;
				player.second->GetGameState().playerAddData.v_fPos = player.second->GetGameState().playerMovementData.v_fPos;
				player.second->GetGameState().playerAddData.score = player.second->GetGameState().score;
				ADDpktLocal << player.second->GetGameState().playerAddData;
				SendTcpPacket(client, ADDpktLocal);
			}

		} break;

		case GameMessages::Login:
		{
			if (m_MapPlayers.count(client->GetID())) {
				printf("Player with ID: %d already exists!\n", client->GetID());
				net::packet<GameMessages> loginRefPkt;
				loginRefPkt << GameMessages::LoginRefuse;
				SendTcpPacket(client, loginRefPkt);
				break;
			}

			shared::LoginInformation* loginInfo;
			loginInfo = packet.GetSubPacketPtr<shared::LoginInformation>();

			bool login_failed = false;
			for (auto ch : loginInfo->sz_unsername) {
				if (ch == 0) break;
				if (ch < 48 || ch > 126) {
					net::packet<GameMessages> loginRefPkt;
					loginRefPkt << GameMessages::LoginRefuse;
					SendTcpPacket(client, loginRefPkt);
					login_failed = true;
					printf("Connection refused BAD USERNAME\n");
					break;
				}
			}

			if (login_failed) break;
			printf("Chosen username: %s\n", loginInfo->sz_unsername);

			std::shared_ptr player = std::make_shared<Player>(client->GetID(), m_Map, this);
			player->SetClient(client);
			player->GetGameState().playerInfo.s_Username.assign(loginInfo->sz_unsername);
			player->GetGameState().playerMovementData.v_fPos = { 100.f, 100.f };
			player->GetGameState().health = 100;
			player->GetGameState().ammo = 30;
			player->GetGameState().score = 0;
			player->GetGameState().playerMovementData.n_uID = client->GetID();

			player->GetGameState().playerAddData.n_uID = client->GetID();
			player->GetGameState().playerAddData.v_fPos = player->GetGameState().playerMovementData.v_fPos;

			std::memcpy(player->GetGameState().playerAddData.sz_Unsername, player->GetGameState().playerInfo.s_Username.c_str(), player->GetGameState().playerInfo.s_Username.size());


#ifdef IPIDS
			m_MapPlayers.insert_or_assign(client->GetIP().to_v4().to_uint(), stGameState);
			std::cout << "New player ID: " << client->GetIP().to_v4().to_uint() << std::endl;
#else
			m_MapPlayers.insert_or_assign(client->GetID(), player);
			std::cout << "New player ID: " << client->GetID() << std::endl;
#endif
			net::packet<GameMessages> loginAccPkt;
			loginAccPkt << GameMessages::LoginAccept;
			SendTcpPacket(client, loginAccPkt);

		} break;

		case GameMessages::UpdatePlayerMovement:
		{
			PlayerMovementData* playerInfo;		//multiple updtates in one packet failes bc everything is in one packet now
			playerInfo = packet.GetSubPacketPtr<PlayerMovementData>();

#ifdef IPIDS
			GameState& playerGameState = m_MapPlayers[client->GetIP().to_v4().to_uint()];
#else
			GameState& playerGameState = m_MapPlayers[client->GetID()]->GetGameState();
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

#ifdef IPIDS
			GameState& playerGameState = m_MapPlayers[client->GetIP().to_v4().to_uint()];
#else
			GameState& playerGameState = m_MapPlayers[client->GetID()]->GetGameState();
#endif
			if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) {
				if (playerGameState.ammo > 0 && playerInfo->playerActionTime - playerGameState.playerObjects.lastShotTime >= m_fExpectedPlayerReloadTime) {
					playerGameState.playerObjects.shotRay = true;	//nah
					playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_ray;
					playerGameState.playerObjects.lastShotTime = playerInfo->playerActionTime;
					--playerGameState.ammo;
				}
				else {
					playerGameState.ammo <= 0 ?
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


			if (playerInfo->u_PlayerActions & (UINT32)PLAYER_ACTION::shot_bullet) {
				playerGameState.playerObjects.bullets.push_back
				({ playerGameState.playerMovementData.v_fPos + Vector2f(cos(Utils::d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f,
					sin(Utils::d2r(playerGameState.playerMovementData.rotation)) * m_fPlayerRadius * 2.f), playerGameState.playerMovementData.rotation });
				playerGameState.playerActionsData.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_bullet;
			}



		}break;

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

void GameServer::UpdateGameState(double currentTime) {
	m_CurrentTime = currentTime;
	g_CurrentTime = currentTime;

	RemoveDisconnectedPlayers();

	for (auto& [playerId, player] : m_MapPlayers)
	{
		player->Udpate(m_MapPlayers);

		if (player->GetGameState().playerMovementData.u_PlayerActions != 0) {
			static net::packet<GameMessages> pkt;
			pkt.header.sendTime = player->GetGameState().packetInfo.cl_PacketDispatchTime;
			pkt.Clear();
			//pkt.header.sendTime = packet.header.sendTime;
			pkt << GameMessages::UpdatePlayerMovement;
			pkt << player->GetGameState().playerMovementData;
			SendTcpPacketToAll(pkt);
			player->GetGameState().playerMovementData.u_PlayerActions = 0;
		}

		//SimulateBullets(playerId, playerGameState);	//breake lag comp for some reason FIXME
	}
}

void GameServer::RemoveDisconnectedPlayers() {
	for (auto& playerID : m_DisconnectedPlayers) {
		if (m_MapPlayers.find(playerID) != m_MapPlayers.end()) {
			m_MapPlayers.erase(playerID);
			net::packet<GameMessages> RMpkt;
			RMpkt << GameMessages::RemovePlayer;
			RMpkt << playerID;
			SendTcpPacketToAll(RMpkt);
		}else
			m_MapPlayers.erase(playerID);
	}
}

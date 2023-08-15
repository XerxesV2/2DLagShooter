#pragma once
#include <NetServer.hpp>
#include <unordered_map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <unordered_set>

#include "Database.hpp"
#include "sv_GameMap.hpp"
#include "LagCompensation.hpp"
#include "PacketStruct.hpp"
#include "utils.hpp"
#include "SharedVariables.hpp"
#include "sv_Player.hpp"
#include "sv_Chat.hpp"


class GameServer : protected PlayerProperies, public net::ServerInterface<GameMessages>
{
public:
	GameServer(uint16_t tcp_port);
	~GameServer();

	void ShutDown();

public:
	std::shared_ptr<Player> GetPlayerById(uint32_t id);

public:
	void UpdateGameState(double currentTime);
	void RemoveDisconnectedPlayers();
	void BanPlayer(uint32_t id, std::string& hwid, long long banTimeInSec);
	GameMap& GetMap() { return m_Map; }
	void UnHittableAntiCheat() {

	}

protected:
	bool OnClientConnect(std::shared_ptr<net::ServerConnection<GameMessages>> tcpClient, std::shared_ptr<net::ServerUdpConnection<GameMessages>> udpClient) override;
	void OnClientDisconnect(const std::shared_ptr<net::ServerConnection<GameMessages>>& client) override;
	void OnUdpPacketReceived(std::shared_ptr<net::ServerUdpConnection<GameMessages>> client, net::udpPacket<GameMessages>& packet) override;
	void OnTcpPacketReceived(std::shared_ptr<net::ServerConnection<GameMessages>> client, net::packet<GameMessages>& packet) override;
	bool OnChecksumMismatch(std::shared_ptr<net::ServerConnection<GameMessages>> client) override;
	void OnChecksumMatch(std::shared_ptr<net::ServerConnection<GameMessages>> client) override;

private:
	void SavePlayerStats();
	bool AddPlayer(std::shared_ptr<net::ServerConnection<GameMessages>> client, shared::LoginInformation* loginInfo);
	void MatchmakerHandlePlayer(Player& player, bool join_or_left);
	static constexpr uint32_t GetTickRate() { return m_uTickRate; }

private:
	GameMap m_Map;
	Database m_Database;

	double m_CurrentTime = 0.0;
	std::unique_ptr<ServerChat> m_pChat;
	std::unordered_map<uint32_t, std::shared_ptr<Player>> m_MapPlayers;
	std::unordered_set<uint32_t> m_DisconnectedPlayers;
	static constexpr uint32_t m_uTickRate = 10;
};


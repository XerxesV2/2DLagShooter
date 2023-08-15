#pragma once
#include <iostream>
#include <deque>
#include <vector>
#include <unordered_map>

#include "NetServerConnection.hpp"
#include "PlayerStats.hpp"
#include "PacketStruct.hpp"
#include "utils.hpp"
#include "sv_GameMap.hpp"
#include "MessageTypes.hpp"

class PlayerProperies
{
protected:
	static constexpr float m_fExpectedPlayerSpeed = 400.f;
	static constexpr float m_fPlayerRadius = 20.f;
	static constexpr float m_fBulletSpeed = 500.f;
	static constexpr float m_fPlayerMaxHealth = 100.f;
	static constexpr float m_fBulletRadius = 10.f;
};

class GameServer;

class Player : protected PlayerProperies
{
public:
	Player(const uint32_t id, GameMap& map, GameServer* const gs);
	~Player();

	void SetClient(std::shared_ptr<net::ServerConnection<GameMessages>>& tcpClient);
	const std::shared_ptr<net::ServerConnection<GameMessages>>& GetTcpClient() { return m_TcpClient; }

	void Udpate(std::unordered_map<uint32_t, std::shared_ptr<Player>>& mapPayers);

	GameState& GetGameState() { return m_PlayerGameState; }
	const GameState& GetGameState() const { return m_PlayerGameState; }
	void CheckIfPlayerDied(GameState& playerGameState, GameState& otherPlayerGameState);

private:
	bool CheckCircleCollide(double x1, double y1, float r1, double x2, double y2, float r2);
	bool TraceRay(GameState& playerGameState, PlayerMovementData& otherPlayerInfo, Vector2f& intersectionPoint);
	void ProcessPlayerShot(GameState& playerGameState, GameState& otherPlayerGameState);
	uint32_t CalcDamageAmount(GameState& hitPlayerGameState, Vector2f rayIntersectionPoint);
	void PreCalcShotRay(GameState& playerGameState);
	void SimulateBullets(const uint32_t playerId, GameState& playerGameState);
	void CheckIfPlayerResponding();

private:
	const uint32_t m_id;
	GameMap& m_Map;

	GameServer* m_Server;
	GameState m_PlayerGameState;
	std::shared_ptr<net::ServerConnection<GameMessages>> m_TcpClient;
	//std::shared_ptr<net::ServerUdpConnection<GameMessages>> m_UdpClient = nullptr;
};


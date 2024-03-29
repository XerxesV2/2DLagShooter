#pragma once
#include "MessageTypes.hpp"
#include "PlayerStruct.hpp"
#include "stdIncludes.hpp"
#include "SharedVariables.hpp"

#include <NetClient.hpp>

class Players;
class GameNetwork : public net::ClientInterface<GameMessages>
{
public:
	GameNetwork(std::shared_ptr<Players>& players, float& deltaTime);
	~GameNetwork();

	void ProcessIncomingPackets();
	void SendPlayerStatusFixedRate();

	bool ConnectToServer();
	bool Connected() { return m_bConnected; }

	void AddPlayerStateToPacketBufferOnFrame();
	void AddPlayerStateToPacketBufferOnTick();
	void TakePlayerActionSnapShot();

private:
	void SendChecksum();

private:
	/* init list */
	std::shared_ptr<Players>& m_pPlayers;
	float& m_fDeltaTime;

	PlayerActionsData m_OnFramePlayerActionsInfo;	//assuming that the reload is longer than fps/tick
	std::unordered_map<uint32_t, PlayerMovementData> m_mapPacketsToVerify;
	uint32_t m_uSequenceNumber = 0u;

	bool m_bConnected{ false };

	const double targetFrameTime = 1.0 / clientUpdateRate;
	double previousTime = 0.0;
	double lagTime = 0.0;

	net::packet<GameMessages> m_GameStatePacket;
};


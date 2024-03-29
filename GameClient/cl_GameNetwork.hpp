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
	void SendLoginInformation(shared::LoginInformation& inf);
	void SendAutoLoginInformation(shared::LoginInformation& inf);
	void RegisterClient();
	void UnRegisterClient();
	GameMessages WaitForLoginReply();
	const long long GetBanTime() const { return m_BanTime; };

	bool ConnectToServer();
	bool Connected() { return IsConnected(); }
	bool LocalPlayerAdded() { return m_bLocalPlayerAdded; }
	bool Accepted() { return m_bAccepted; }

	void AddPlayerStateToPacketBufferOnFrame();
	void AddPlayerStateToPacketBufferOnTick();
	void TakePlayerActionSnapShot();
	void SendChatMessage(const char * msg, const size_t size);
	void SendPrivateChatMessage(const char * msg, const size_t size, const uint32_t id);
	void SendChatUserCommand(const shared::UserCommand& usercmd);
	void PushFlagDropAction();

private:
	void SendChecksum();

private:
	/* init list */
	std::shared_ptr<Players>& m_pPlayers;
	float& m_fDeltaTime;

	PlayerActionsData m_OnFramePlayerActionsInfo;	//assuming that the reload is longer than fps/tick
	std::unordered_map<uint32_t, PlayerMovementData> m_mapPacketsToVerify;
	uint32_t m_uSequenceNumber = 0u;

	bool m_bLocalPlayerAdded{ false };
	bool m_bAccepted{ false };
	long long m_BanTime{ 0 };

	const double targetFrameTime = 1.0 / clientUpdateRate;
	double previousTime = 0.0;
	double lagTime = 0.0;

	net::udpPacket<GameMessages> m_UdpGameStatePacket;
	net::packet<GameMessages> m_TcpGameStatePacket;
};


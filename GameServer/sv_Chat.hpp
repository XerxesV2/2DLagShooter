#pragma once
#include "PacketStruct.hpp"

#include <NetServerConnection.hpp>
#include <memory>

class GameServer;

class ServerChat
{
public:
	ServerChat(GameServer* const gs);
	~ServerChat();

	void HandleIncomingMessage(std::shared_ptr<net::ServerConnection<GameMessages>> client, ChatMessageData* const chatMsgData);
	void HandleUserCommand(std::shared_ptr<net::ServerConnection<GameMessages>> client, shared::UserCommand* const chatMsgData);

private:
	void LoadDirtyWords();
	bool FilterMessage(ChatMessageData* const chatMsgData);

private:
	GameServer* m_Server;

	std::vector<std::string> m_DirtyWords;
	/*chatbann uid : ban time*/
	std::unordered_map<uint32_t, double> m_BannedPlayers;
};


#define _CRT_SECURE_NO_WARNINGS
#include "sv_Chat.hpp"
#include "Server.hpp"

#include <fstream>

ServerChat::ServerChat(GameServer* const gs)
{
	std::srand((uint32_t)g_CurrentTime);
	m_Server = gs;
	LoadDirtyWords();
}

ServerChat::~ServerChat()
{
}

void ServerChat::HandleIncomingMessage(std::shared_ptr<net::ServerConnection<GameMessages>> client, ChatMessageData* const chatMsgData)
{
	if (FilterMessage(chatMsgData))
	{
		chatMsgData->n_uID = 1;
		std::string suprise("Ne beszelj csunyan ");
		suprise.append(m_DirtyWords.at(std::rand() % m_DirtyWords.size()));
		std::transform(suprise.begin(), suprise.end(), suprise.begin(), std::tolower);
		suprise.at(0) = std::toupper(suprise.at(0));
		suprise.append("!");

		memcpy(chatMsgData->msg, suprise.c_str(), suprise.size());
		net::packet<GameMessages> msg_pkt;
		msg_pkt << GameMessages::ChatMessage;
		msg_pkt << *chatMsgData;
		m_Server->SendTcpPacket(client, msg_pkt);
	}
	else 
	{
		net::packet<GameMessages> msg_pkt;
		msg_pkt << GameMessages::ChatMessage;
		msg_pkt << *chatMsgData;
		m_Server->SendTcpPacketToAll(msg_pkt, client);
	}
}

void ServerChat::LoadDirtyWords()
{
	std::ifstream is("dirty_words_e.txt");

	std::string word;
	while (std::getline(is, word))
	{
		std::transform(word.begin(), word.end(), word.begin(), std::toupper);
		m_DirtyWords.push_back(word);
		//m_DirtyWords.push_back(std::move(word));
	}
}

bool ServerChat::FilterMessage(ChatMessageData* const chatMsgData)
{
	std::string msg(chatMsgData->msg);
	std::transform(msg.begin(), msg.end(), msg.begin(), std::toupper);
	for (auto& dirtyWord: m_DirtyWords) {
		if (msg.find(dirtyWord) != std::string::npos)
			return true;
	}
	return false;
}

void ServerChat::HandleUserCommand(std::shared_ptr<net::ServerConnection<GameMessages>> client, shared::UserCommand* const userCmdData)
{
	std::shared_ptr<Player> pLPlayer = m_Server->GetPlayerById(userCmdData->uIDLeft);
	if (pLPlayer == nullptr) return;

	shared::UserCommand userCmdCpy = *userCmdData;

	net::packet<GameMessages> statUpdatePkt;
	statUpdatePkt << GameMessages::UpdateStats;
	userCmdData->uIDRight = userCmdData->uIDLeft;
	userCmdData->uIDLeft = client->GetID();
	statUpdatePkt << *userCmdData;

	switch (userCmdData->uUserCommand)
	{
	case shared::UserCmd::ADD_AMMO:
	{
		pLPlayer->GetGameState().ammo += userCmdData->iValue;
		m_Server->SendTcpPacket(pLPlayer->GetTcpClient(), statUpdatePkt);
	} break;
	case shared::UserCmd::KILL:
	case shared::UserCmd::ADD_HEALTH:
	{
		pLPlayer->GetGameState().health += userCmdData->iValue;
		m_Server->SendTcpPacketToAll(statUpdatePkt);
		std::shared_ptr<Player> pp = m_Server->GetPlayerById(client->GetID());
		if (pLPlayer != nullptr)
			pLPlayer->CheckIfPlayerDied(pp->GetGameState(), pLPlayer->GetGameState());
	} break;

	case shared::UserCmd::TELEPORT:
	{
		std::shared_ptr<Player> pRPlayer = m_Server->GetPlayerById(userCmdCpy.uIDRight);
		pLPlayer->GetGameState().playerMovementData.v_fPos = pRPlayer->GetGameState().playerMovementData.v_fPos;
		net::packet<GameMessages> posUpdatePkt;
		posUpdatePkt << GameMessages::UpdatePlayerMovement;
		posUpdatePkt << pLPlayer->GetGameState().playerMovementData;
		m_Server->SendTcpPacketToAll(posUpdatePkt);
		printf("Player [%s] teleported to [%s]\n" , pLPlayer->GetGameState().playerInfo.s_Username, pRPlayer->GetGameState().playerInfo.s_Username);
	} break;

	default: break;
	}
}

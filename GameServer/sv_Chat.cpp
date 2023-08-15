#include "sv_Chat.hpp"
#include "Server.hpp"

#include <fstream>
#include <algorithm>

ServerChat::ServerChat(GameServer* const gs)
{
	std::srand(time(0));
	m_Server = gs;
	LoadDirtyWords();
}

ServerChat::~ServerChat()
{
}

void ServerChat::HandleIncomingMessage(std::shared_ptr<net::ServerConnection<GameMessages>> client, ChatMessageData* const chatMsgData)
{
	if (chatMsgData->fMsgFlags == (uint8_t)FLAGS_CHATMSG::private_msg)
	{
		std::shared_ptr<Player> pLPlayer = m_Server->GetPlayerById(chatMsgData->n_uID);
		chatMsgData->n_uID = client->GetID();
		net::packet<GameMessages> msg_pkt;
		msg_pkt << GameMessages::ChatMessage;
		msg_pkt << *chatMsgData;
		m_Server->SendTcpPacket(pLPlayer->GetTcpClient(), msg_pkt);
		return;
	}

	if (FilterMessage(chatMsgData) && m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.rank != shared::PlayerRank::OWNER)
	{
		chatMsgData->n_uID = 1;
		std::string suprise("Ne beszelj csunyan ");
		suprise.append(m_DirtyWords.at(std::rand() % m_DirtyWords.size()));
		std::transform(suprise.begin(), suprise.end(), suprise.begin(), ::tolower);
		suprise.at(0) = std::toupper(suprise.at(0));
		suprise.append("!");

		memcpy(chatMsgData->msg, suprise.c_str(), suprise.size());
		chatMsgData->fMsgFlags = (uint8_t)FLAGS_CHATMSG::moderate_msg;
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
		m_Server->SendTcpPacketToAll(client->GetGroupID(), msg_pkt);
	}
}

void ServerChat::LoadDirtyWords()
{
	std::ifstream is("dirty_words_e.txt");

	std::string word;
	while (std::getline(is, word))
	{
		std::transform(word.begin(), word.end(), word.begin(), ::toupper);
		m_DirtyWords.push_back(word);
		//m_DirtyWords.push_back(std::move(word));
	}
}

bool ServerChat::FilterMessage(ChatMessageData* const chatMsgData)
{
	std::string msg(chatMsgData->msg);
	std::transform(msg.begin(), msg.end(), msg.begin(), ::toupper);
	for (auto& dirtyWord: m_DirtyWords) {
		if (msg.find(dirtyWord) != std::string::npos)
			return true;
	}
	return false;
}

void ServerChat::HandleUserCommand(std::shared_ptr<net::ServerConnection<GameMessages>> client, shared::UserCommand* const userCmdData)
{
	//temp
	if (m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.rank > shared::PlayerRank::OP) {
		ChatMessageData svMsg;
		svMsg.n_uID = 1;
		strcpy_s(svMsg.msg, "You dont have permission to use this command!");
		net::packet<GameMessages> msg_pkt;
		msg_pkt << GameMessages::ChatMessage;
		msg_pkt << svMsg;
		m_Server->SendTcpPacket(client, msg_pkt);

		return;
	}

	std::shared_ptr<Player> pLPlayer = m_Server->GetPlayerById(userCmdData->uIDLeft);
	if (pLPlayer == nullptr) return;

	shared::UserCommand userCmdCpy = *userCmdData;
	userCmdCpy.uIDRight = userCmdData->uIDLeft;
	userCmdCpy.uIDLeft = client->GetID();
	userCmdCpy.iValue = (int)pLPlayer->GetGameState().playerInfo.rank;

	net::packet<GameMessages> statusUpdatePkt;
	statusUpdatePkt << GameMessages::UpdateStatus;
	
	if (userCmdData->uUserCommand == shared::UserCmd::SET_RANK)
	{
		if (userCmdData->iValue == (int)shared::PlayerRank::OWNER) {
			printf("Ezhulye xdddd get banned\n");
			m_Server->BanPlayer(client->GetID(), m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.s_Hwid, -1LL);
			return;
		}
		if (pLPlayer->GetGameState().playerInfo.rank <= m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.rank
			&& m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.rank != shared::PlayerRank::OWNER)
		{
			ChatMessageData svMsg;
			svMsg.n_uID = 1;
			strcpy_s(svMsg.msg, "Your rank is not hight enough to do that.");
			net::packet<GameMessages> msg_pkt;
			msg_pkt << GameMessages::ChatMessage;
			msg_pkt << svMsg;
			m_Server->SendTcpPacket(client, msg_pkt);
			return;
		}

		pLPlayer->GetGameState().playerInfo.rank = (shared::PlayerRank)userCmdData->iValue;
		printf("-= [%s] rank has changed to (%d) =-\n", pLPlayer->GetGameState().playerInfo.s_Username.c_str(), userCmdData->iValue);
		userCmdCpy.iValue = (int)pLPlayer->GetGameState().playerInfo.rank;
		statusUpdatePkt << userCmdCpy;
		m_Server->SendTcpPacketToAll(client->GetGroupID(), statusUpdatePkt);
		return;
	}

	if (pLPlayer->GetGameState().playerInfo.rank < m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.rank
		&& m_Server->GetPlayerById(client->GetID())->GetGameState().playerInfo.rank != shared::PlayerRank::OWNER) {
		ChatMessageData svMsg;
		svMsg.n_uID = 1;
		strcpy_s(svMsg.msg, "You can only do that with players of lower rank than you.");
		net::packet<GameMessages> msg_pkt;
		msg_pkt << GameMessages::ChatMessage;
		msg_pkt << svMsg;
		m_Server->SendTcpPacket(client, msg_pkt);
		return;
	}

	userCmdCpy.iValue = userCmdData->iValue;
	statusUpdatePkt << userCmdCpy;

	switch (userCmdData->uUserCommand)
	{
	case shared::UserCmd::ADD_AMMO:
	{
		pLPlayer->GetGameState().stats.ammo += userCmdData->iValue;
		m_Server->SendTcpPacket(pLPlayer->GetTcpClient(), statusUpdatePkt);
	} break;
	case shared::UserCmd::KILL:
	case shared::UserCmd::ADD_HEALTH:
	{
		pLPlayer->GetGameState().stats.health += userCmdData->iValue;
		m_Server->SendTcpPacketToAll(client->GetGroupID(), statusUpdatePkt);
		std::shared_ptr<Player> pp = m_Server->GetPlayerById(client->GetID());
		if (pLPlayer != nullptr)
			pLPlayer->CheckIfPlayerDied(pp->GetGameState(), pLPlayer->GetGameState());
	} break;

	case shared::UserCmd::TELEPORT:
	{
		std::shared_ptr<Player> pRPlayer = m_Server->GetPlayerById(userCmdData->uIDRight);
		pLPlayer->GetGameState().playerMovementData.v_fPos = pRPlayer->GetGameState().playerMovementData.v_fPos;
		net::packet<GameMessages> posUpdatePkt;
		posUpdatePkt << GameMessages::UpdatePlayerMovement;
		posUpdatePkt << pLPlayer->GetGameState().playerMovementData;
		m_Server->SendTcpPacketToAll(client->GetGroupID(), posUpdatePkt);
		printf("Player [%s] teleported to [%s]\n" , pLPlayer->GetGameState().playerInfo.s_Username.c_str(), pRPlayer->GetGameState().playerInfo.s_Username.c_str());
	} break;
	case shared::UserCmd::BAN:
	{
		printf("Player [%s] banned for %f hours\n", pLPlayer->GetGameState().playerInfo.s_Username.c_str(), ((float)userCmdData->iValue/(60.f*60.f)));
		m_Server->BanPlayer(pLPlayer->GetGameState().playerInfo.u_id, pLPlayer->GetGameState().playerInfo.s_Hwid, (long long)userCmdData->iValue);
	}break;

	default: break;
	}
}
#include "cl_Chat.hpp"
#include "cl_GameNetwork.hpp"
#include "cl_Players.hpp"

GameNetwork::GameNetwork(std::shared_ptr<Players>& players, float& deltaTime) : m_pPlayers(players), m_fDeltaTime(deltaTime)
{
}

GameNetwork::~GameNetwork()
{
}

void GameNetwork::ProcessIncomingPackets()
{
    if (!IsConnected()) return;
    while (!IncomingPackets().Empty()) {
        auto pkt = IncomingPackets().PopFront().pkt;

        while (pkt.bytesRead < pkt.body.size())
        {
            switch (pkt.GetNextSubPacketType())
            {

                case GameMessages::SetClientID: {
                    uint32_t* id;
                    id = pkt.GetSubPacketPtr<uint32_t>();
                    m_pPlayers->SetLocalPLayerId(*id);
                    std::cout << "Your ID is: " << *id << "\n";
                    break;
                }

                case GameMessages::AddPlayer: {
                    static AddPlayerData* addData;
                    addData = pkt.GetSubPacketPtr<AddPlayerData>();
                    m_pPlayers->InitPlayer(*addData);
                    if (addData->n_uID == m_pPlayers->GetLocalPLayerId()) {   //should be the last player
                        m_pPlayers->AddLocalPlayer(*addData);
                        m_bLocalPlayerAdded = true;
                    }

                    Chat::Get().AddPlayerNameToPlayerList(addData->n_uID, m_pPlayers->GetPLayerById(addData->n_uID)->stats.username);
                    break;
                }

                case GameMessages::RemovePlayer: {
                    uint32_t* idToRemove;
                    idToRemove = pkt.GetSubPacketPtr<uint32_t>();
                    Chat::Get().RemovePlayerFromPlayerList(m_pPlayers->GetPLayerById(*idToRemove)->stats.username);
                    m_pPlayers->RemovePlayer(*idToRemove);

                    break;
                }

                case GameMessages::UpdatePlayerMovement:
                {
                    if (!m_bLocalPlayerAdded) return;
                    //static PlayerStruct sPlayer;
                    PlayerMovementData* movementInfo;
                    movementInfo = pkt.GetSubPacketPtr<PlayerMovementData>();

                    if (movementInfo->n_uID == m_pPlayers->GetLocalPLayerId()) { //local player
                        if (m_mapPacketsToVerify.empty()) break;
                        if (m_mapPacketsToVerify[movementInfo->n_uSequence].v_fPos != movementInfo->v_fPos) {
                            m_pPlayers->RegulateLocalPlayer(*movementInfo);  //[FIXED ig] rare rubber bandig when other player shot, must be server 
                            m_mapPacketsToVerify.erase(movementInfo->n_uSequence);
                        }
                        else {
                            //std::cout << "size: " << m_mapPacketsToVerify.size() << std::endl;
                            m_pPlayers->CalcPing(*movementInfo, pkt.header.sendTime);
                            m_mapPacketsToVerify.erase(movementInfo->n_uSequence);
                        }
                    }
                    else {
                        m_pPlayers->ProcessOtherPlayerMovement(*movementInfo);
                        m_pPlayers->CalcPing(*movementInfo, pkt.header.sendTime);
                    }

                    //std::cout << "Player updated with ID: " << sPlayer.n_uID << "\n";
                    break;
                }
                case GameMessages::UpdatePlayerActions:
                {
                    if (!m_bLocalPlayerAdded) return;

                    PlayerActionsData* actionInfo;
                    actionInfo = pkt.GetSubPacketPtr<PlayerActionsData>();

                    m_pPlayers->ProcessOtherPlayerActions(*actionInfo);

                    break;
                }

                case GameMessages::GameStateUpdate:
                {
                    if (!m_bLocalPlayerAdded) return;

                    sv_HitregData* sHitreg;
                    sHitreg = pkt.GetSubPacketPtr<sv_HitregData>();
                    m_pPlayers->UpdateGameStateVariables(*sHitreg);
                    break;
                }
                case GameMessages::PlayerDied:
                {
                    sv_PLayerDiedData* sPlayerData;
                    sPlayerData = pkt.GetSubPacketPtr<sv_PLayerDiedData>();

                    m_mapPacketsToVerify.clear(), m_uSequenceNumber = 0;

                    m_pPlayers->HandlePlayerKill(*sPlayerData);

                    break;
                }
                case GameMessages::ChatMessage:
                {
                    ChatMessageData* chatMsgData;
                    chatMsgData = pkt.GetSubPacketPtr<ChatMessageData>();

                    if(chatMsgData->n_uID != 1)
                        Chat::Get().AppendMessage(m_pPlayers->GetPLayerById(chatMsgData->n_uID), chatMsgData->msg);
                    else
                        Chat::Get().AppendMessage(nullptr, chatMsgData->msg);

                }break;
                case GameMessages::UpdateStats:
                {
                    shared::UserCommand* userCmdData;
                    userCmdData = pkt.GetSubPacketPtr<shared::UserCommand>();
                    m_pPlayers->HandleStatUdpate(userCmdData);

                }break;
                default: printf("\nWrong tcp packet data type!\n");
            }
        }
    }

    while (!IncomingUdpPackets().Empty()) {
        auto pkt = IncomingUdpPackets().PopFront().pkt;

        while (pkt.bytesRead < pkt.body.size())
        {
            switch (pkt.GetNextSubPacketType())
            {

            case GameMessages::UpdatePlayerMovement:
            {
                if (!m_bLocalPlayerAdded) return;
                //static PlayerStruct sPlayer;
                PlayerMovementData* movementInfo;
                movementInfo = pkt.GetSubPacketPtr<PlayerMovementData>();

                if (movementInfo->n_uID == m_pPlayers->GetLocalPLayerId()) { //local player
                    if (m_mapPacketsToVerify.empty()) break;
                    if (m_mapPacketsToVerify[movementInfo->n_uSequence].v_fPos != movementInfo->v_fPos) {
                        m_pPlayers->RegulateLocalPlayer(*movementInfo);  //[FIXED ig] rare rubber bandig when other player shot, must be server 
                        m_mapPacketsToVerify.erase(movementInfo->n_uSequence);
                    }
                    else {
                        //std::cout << "size: " << m_mapPacketsToVerify.size() << std::endl;
                        m_pPlayers->CalcPing(*movementInfo, pkt.header.sendTime);
                        m_mapPacketsToVerify.erase(movementInfo->n_uSequence);
                    }
                }
                else {
                    m_pPlayers->ProcessOtherPlayerMovement(*movementInfo);
                    m_pPlayers->CalcPing(*movementInfo, pkt.header.sendTime);
                }

                //std::cout << "Player updated with ID: " << sPlayer.n_uID << "\n";
                break;
            }
            case GameMessages::UpdatePlayerActions:
            {
                if (!m_bLocalPlayerAdded) return;

                PlayerActionsData* actionInfo;
                actionInfo = pkt.GetSubPacketPtr<PlayerActionsData>();

                m_pPlayers->ProcessOtherPlayerActions(*actionInfo);

                break;
            }

            case GameMessages::UdpTest:
            {
                printf("Test udp arrived...\n");
            } break;

            default: printf("\nWrong udp packet data type!");
            }
        }
    }
}

void GameNetwork::SendPlayerStatusFixedRate()
{

    AddPlayerStateToPacketBufferOnFrame();
    TakePlayerActionSnapShot();
    //m_pPlayers->PacketSent();

    double currentTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    double elapsedTime = currentTime - previousTime;
    previousTime = currentTime;

    lagTime += elapsedTime;

    if (lagTime >= targetFrameTime) 
    {
        //AddPlayerStateToPacketBufferOnTick();

        m_GameStatePacket.header.sendTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        if (!m_GameStatePacket.body.empty()) SendUdpPacket(m_GameStatePacket), ++m_uSequenceNumber;

        m_GameStatePacket.Clear();

        lagTime = 0.0;
    }

}

void GameNetwork::SendLoginInformation(shared::LoginInformation& inf)
{
    net::packet<GameMessages> loginPkt;
    loginPkt << GameMessages::Login;
    loginPkt << inf;
    SendTcpPacket(loginPkt);
}

void GameNetwork::RegisterClient()
{
    net::packet<GameMessages> pkt;
    pkt << GameMessages::RegisterClient;
    SendTcpPacket(pkt);
}

GameMessages GameNetwork::WaitForLoginReply()
{
    if (!IsConnected()) return GameMessages::None;
    while (!IncomingPackets().Empty()) {
        auto pkt = IncomingPackets().PopFront().pkt;

        while (pkt.bytesRead < pkt.body.size())
        {
            switch (pkt.GetNextSubPacketType())
            {
            case GameMessages::ServerAccept: {
                std::cout << "Server accepted!\n";
                m_bAccepted = true;
                return GameMessages::ServerAccept;
                break;
            }

            case GameMessages::ServerRefuse: {
                std::cout << "Server refused :(\n";
                Disconnect();
                return GameMessages::ServerRefuse;
                break;
            }
            case GameMessages::LoginAccept: {
                std::cout << "Login success\n";
                return GameMessages::LoginAccept;
                break;
            }
            case GameMessages::LoginRefuse: {
                return GameMessages::LoginRefuse;
                break;
            }
            case GameMessages::AssignUdpPort: {
                uint16_t* port;
                port = pkt.GetSubPacketPtr<uint16_t>();
                SetUdp(*port);
                std::cout << "UDP port assigned: " << *port << "\n";
                break;
            }
            default: printf("\n[LOGIN] Wrong tcp packet data type!"); break;
            }
        }
    }

    return GameMessages::None;
}

bool GameNetwork::ConnectToServer() {
    if (!Connect("127.0.0.1", 6969)) {
        std::cout << "Server is down\n";
        return EXIT_FAILURE;
    }
    SendChecksum();
}

void GameNetwork::AddPlayerStateToPacketBufferOnFrame()
{
    PlayerStruct* const localPlayer = m_pPlayers->GetLocalPLayer();

    if (1) {
        m_mapPacketsToVerify[m_uSequenceNumber] = localPlayer->st_PlayerMovementInfo;
       
        localPlayer->st_PlayerMovementInfo.n_uSequence = m_uSequenceNumber;
        localPlayer->st_PlayerMovementInfo.deltaTime = m_fDeltaTime;

        m_GameStatePacket << GameMessages::UpdatePlayerMovement;
        m_GameStatePacket << localPlayer->st_PlayerMovementInfo;

        localPlayer->st_PlayerMovementInfo.u_PlayerActions = 0;
    }

    if (m_OnFramePlayerActionsInfo.u_PlayerActions != 0) {

        m_GameStatePacket << GameMessages::UpdatePlayerActions;
        m_GameStatePacket << m_OnFramePlayerActionsInfo;
        m_OnFramePlayerActionsInfo.u_PlayerActions = 0;
    }
}

void GameNetwork::AddPlayerStateToPacketBufferOnTick() {
    
}

void GameNetwork::TakePlayerActionSnapShot()    //must save the action at frame for later process
{
    PlayerStruct* const localPlayer = m_pPlayers->GetLocalPLayer();
    if (localPlayer->st_PlayerActionsInfo.u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) { //>> same as &
        m_OnFramePlayerActionsInfo = localPlayer->st_PlayerActionsInfo;
        localPlayer->st_PlayerActionsInfo.u_PlayerActions = 0;
    }
}

void GameNetwork::SendChatMessage(const char* msg, const size_t size)
{
    ChatMessageData chatMsgData;
    chatMsgData.n_uID = m_pPlayers->GetLocalPLayerId();
    std::memcpy(chatMsgData.msg, msg, size);

    net::packet<GameMessages> msg_pkt;
    msg_pkt << GameMessages::ChatMessage;
    msg_pkt << chatMsgData;
    SendTcpPacket(msg_pkt);
}

void GameNetwork::SendChatUserCommand(const shared::UserCommand& usercmd)
{
    net::packet<GameMessages> msg_pkt;
    msg_pkt << GameMessages::ChatCommand;
    msg_pkt << usercmd;
    SendTcpPacket(msg_pkt);
}

void GameNetwork::SendChecksum()
{
    net::packet<GameMessages> pkt;
    char hs[65]{};
    pkt << hs;
    SendTcpPacket(pkt);
}

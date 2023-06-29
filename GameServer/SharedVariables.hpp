#pragma once

inline constexpr double serverTickRate = 60.0;
inline constexpr double clientUpdateRate = 30.0;
#define maxChatMessageLength 128u //KURVA CONSTEXPR NEM MEGY

inline constexpr float g_RayLength = 1920.f;
inline constexpr float g_PlayerRadius = 20.f;

inline constexpr int g_KillReward_ammo = 3;
inline constexpr int g_KillReward_score = 100;

inline constexpr float g_CamSizeX = 1920.f;
inline constexpr float g_CamSizeY = 1080.f;

struct BBox
{
    float left;
    float top;
    float right;
    float buttom;
} constexpr m_MapBB = { 0.f, 0.f, 1920.f, 1080.f };

namespace shared
{
    struct LoginInformation
    {
        char sz_unsername[20];
    };

    enum class UserCmd : unsigned int
    {
        ADD_AMMO = 100U,
        ADD_HEALTH,
        TELEPORT,
        KILL,
        BAN,
        HELP,

        NONE
    };

    struct UserCommand
    {
        unsigned int uIDLeft = 0;
        unsigned int uIDRight = 0;
        UserCmd uUserCommand = UserCmd::NONE;
        int iValue = 0;
    };
}
// buta AI
/*
    Client                                  Server
   --------                                --------
      |                                        |
      |          Player movements              |
      |--------------------------------------->|   (1) Player updates
      |                                        |
      |     Interpolate between updates        |
      |<---------------------------------------|   (2) Smoothed player movements
      |                                        |
      |       Player fires a shot              |
      |--------------------------------------->|   (3) Shot information
      |                                        |
      | Extrapolate player positions           |
      | Based on view interpolation and        |
      | interpolation delay                    |
      |                                        |
      |                                        |
      |          Lag compensated               |
      |         shot calculation               |
      |                                        |
*/

/*
 Client                                   Server
     +-----------------+                      +-----------------+
     |                 |       User Input     |                 |
     |  Prediction     |   -----------------> |  Simulation     |
     |                 |      (Commands)      |                 |
     +-----------------+                      +-----------------+
                |                                      |
                |  Network                           Network
                |                                      |
                V                                      V
     +-----------------+                      +-----------------+
     |                 |    Server Snapshot   |                 |
     |  Interpolation  |  <-----------------  |  Reconciliation |
     |                 |  (Snapshot + States) |                 |
     +-----------------+                      +-----------------+
                |                                      |
                |  Network                           Network
                |                                      |
                V                                      V
     +-----------------+                      +-----------------+
     |                 |   Client Snapshot    |                 |
     |  Interpolation  |  <-----------------  |  Reconciliation |
     |                 |  (Snapshot + States) |                 |
     +-----------------+                      +-----------------+
*/

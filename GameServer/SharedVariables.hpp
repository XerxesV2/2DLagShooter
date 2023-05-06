#pragma once

inline double serverTickRate = 60.0;
inline double clientUpdateRate = 30.0;

inline float g_RayLength = 1920.f;
inline float g_PlayerRadius = 20.f;

struct BBox
{
    float left;
    float top;
    float right;
    float buttom;
} constexpr m_MapBB = { 0.f, 0.f, 1920.f, 1080.f };

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

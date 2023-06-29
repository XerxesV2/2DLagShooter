#pragma once
#include "cl_GameMap.hpp"
#include <SFML/Graphics.hpp>

#include "PacketStruct.hpp"
#include "stdIncludes.hpp"
#include "utils.hpp"
#include "PlayerStruct.hpp"
#include "cl_Globals.hpp"

class Players : public sf::Drawable
{
public:
	Players(sf::RenderWindow& _window, sf::Font& font, float& deltaTime);
	~Players();

	void Update();

    /* Network */
    void InitPlayer(AddPlayerData& addData);
    void AddLocalPlayer(AddPlayerData& addData);
    void RemovePlayer(uint32_t id);
    void RegulateLocalPlayer(PlayerMovementData& playerInfo);
    void ProcessOtherPlayerMovement(PlayerMovementData& playerInfo);
    void ProcessOtherPlayerActions(PlayerActionsData& playerInfo);
    void UpdateGameStateVariables(sv_HitregData& hitregInfo);
    void HandlePlayerKill(sv_PLayerDiedData& playerStateinfo);
    void HandleStatUdpate(shared::UserCommand* usercmd);
    void CalcPing(const PlayerMovementData& playerInfo, float RTT);

    const bool ShouldSendPacket() const { return m_bShouldSendPacket; }
    PlayerStruct* const GetLocalPLayer() const { return m_pLocalPlayer; }
    PlayerStruct* const GetPLayerById(const uint32_t id) { return &m_mapPlayers[id]; }
    const PlayerStatsStruct* GetLocalPLayerStat() const { return &m_pLocalPlayer->stats; }
    const uint32_t GetLocalPLayerId() const { return m_LocalPlayerID; }
    void SetLocalPLayerId(uint32_t id) { m_LocalPlayerID = id; }
    void PacketSent() { m_bShouldSendPacket = false; }
    double m_DeltaTimeTimePoint = 0.0;

    /* Options */
    void EnableInterpolation();

private:
    void UpdateLocalPlayer();
    void UpdateOtherPlayers();
    void UpdateObjects();
    void UpdateStatText(PlayerStruct& player);
    void UpdateLocalPlayerActions();
    void UpdatePlayerBeams(PlayerStruct& player);

    void SetPlayerPos(uint32_t id, sf::Vector2f pos);
    void SetPlayerPos(uint32_t id, float x, float y);
    void SetPlayerPos(PlayerStruct& player, float x, float y);
    void MovePlayer(uint32_t id, sf::Vector2f amount);
    void MovePlayer(uint32_t id, float x, float y);
    void MovePlayer(PlayerStruct& player, float x, float y);

    void MovePlayerToPointFixTime(PlayerStruct& player, const Vector2f& targetPosition);
    void MovePlayerToPointFixSpeed(PlayerStruct& player, const Vector2f& targetPosition);

    void UpdateCheats();
    void TeleportHack();
    void DeltaTimeManipulationSpeedHack();
    void RapidFireHack();
    void BotMove();

    void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;
    
private:
    /* init list */
    sf::RenderWindow& window;
    sf::Font& m_Font;
    float& m_fDeltaTime;

    uint32_t m_LocalPlayerID{ 0 };
    GameMap m_Map;

    std::unordered_map<uint32_t, PlayerStruct> m_mapPlayers;
    PlayerStruct* m_pLocalPlayer = nullptr;

    sf::CircleShape hitMarker;
    sf::CircleShape lagCompensatedPlayerGhost;
    sf::CircleShape realPlayerGhost;
    sf::CircleShape realLocalPlayerGhost;
    sf::CircleShape serverUpdatedPlayerPos;

    sf::Clock m_coolDown;
    sf::Clock m_clTime;
    bool m_bShouldSendPacket{ false };
    bool m_bShouldCheckBulletCollision{ false };
    bool m_bRecalcInterpolationSpeed{ true };
    bool m_bInterpolate{ true };

    float m_fBeamDuration = 1.f;
    float m_fBeamFadeTime = 2.f;
    float m_fFireRate = 0.5f;
    static constexpr float m_fSpeed = 400.f;
    static constexpr float m_fBulletSpeed = 500.f;
    static constexpr float m_fPlayerRadius = 20.f;
};
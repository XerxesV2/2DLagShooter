#include "cl_Chat.hpp"
#include "cl_Players.hpp"
#include "SharedVariables.hpp"
#include "UI/LogSystem.hpp"
#include "UI/LeaderBoard.hpp"
#include "UI/Killfeed.hpp"
#include <sstream>

//#define DEBUG

Players::Players(sf::RenderWindow& _window, sf::Font& font, float& deltaTime) : window(_window), m_Font(font), m_fDeltaTime(deltaTime)
{
    hitMarker.setRadius(4.f);
    hitMarker.setOrigin(4.f, 4.f);
    hitMarker.setFillColor(sf::Color(50, 50, 250, 255));

    lagCompensatedPlayerGhost.setRadius(m_fPlayerRadius);
    lagCompensatedPlayerGhost.setOrigin(m_fPlayerRadius, m_fPlayerRadius);
    lagCompensatedPlayerGhost.setFillColor(sf::Color(0, 0, 255, 100));

    realPlayerGhost.setRadius(m_fPlayerRadius);
    realPlayerGhost.setOrigin(m_fPlayerRadius, m_fPlayerRadius);
    realPlayerGhost.setFillColor(sf::Color(255, 0, 0, 100));

    realLocalPlayerGhost.setRadius(m_fPlayerRadius);
    realLocalPlayerGhost.setOrigin(m_fPlayerRadius, m_fPlayerRadius);
    realLocalPlayerGhost.setFillColor(sf::Color(0, 255, 0, 100));

    serverUpdatedPlayerPos.setRadius(10.f);
    serverUpdatedPlayerPos.setOrigin(10.f, 10.f);
    serverUpdatedPlayerPos.setFillColor(sf::Color::Yellow);

    if (g_bOfflinePLay) {
        m_LocalPlayerID = 1000;
        AddPlayerData addData;
        addData.n_uID = 1000;
        addData.v_fPos = { 100.f ,100.f };
        InitPlayer(addData);
        AddLocalPlayer(addData);
    }

}

Players::~Players()
{
}

void Players::Update()
{
    UpdateLocalPlayer();
    UpdateOtherPlayers();
    UpdateObjects();

    //if (m_bShouldSendPacket) m_pNetwork->AddPlayerStateToPacketBuffer();
}

void Players::EnableInterpolation()
{
    m_bInterpolate = !m_bInterpolate;
    LogSystem::Get().PrintMessage("Interpolation", sf::Color(90, 90, 100, 255));
    LogSystem::Get().PrintMessage(m_bInterpolate ? "on" : "off", m_bInterpolate ? sf::Color::Green : sf::Color::Red, true);
}

void Players::ChangeMouseRelative()
{
    m_bRelativeMouse = !m_bRelativeMouse;
    LogSystem::Get().PrintMessage("The aim is ", sf::Color(90, 90, 100, 255));
    LogSystem::Get().PrintMessage(m_bInterpolate ? "ralative" : "not ralative", m_bInterpolate ? sf::Color::Yellow : sf::Color::Blue, true);
}

void Players::UpdateLocalPlayer()
{
    UpdateCheats();
    if (!window.hasFocus() || Chat::Get().IsActive()) return;

    /* -= MOVEMENT UPDATE =- */
    m_pLocalPlayer->dirVec = { 0.f, 0.f };
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        m_pLocalPlayer->dirVec.x =  -1.f, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_left;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        m_pLocalPlayer->dirVec.x =  1.f, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions  |= (UINT32)PLAYER_ACTION::move_right;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        m_pLocalPlayer->dirVec.y =  -1.f, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_up;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        m_pLocalPlayer->dirVec.y =  1.f, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions  |= (UINT32)PLAYER_ACTION::move_down;

    Vector2f movementAmount = m_pLocalPlayer->dirVec * m_fSpeed * m_fDeltaTime;
    m_pLocalPlayer->st_PlayerMovementInfo.v_fPos += movementAmount;

    m_Map.ArrangePlayerCollision(m_pLocalPlayer->st_PlayerMovementInfo.v_fPos);

    SetPlayerPos(m_LocalPlayerID, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.x, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.y);

    if (!m_bRelativeMouse && !m_bMouseMoved)
        sf::Mouse::setPosition(sf::Mouse::getPosition() + sf::Vector2i(movementAmount.x, movementAmount.y));
    else
        m_bMouseMoved = false;

    /* -= FLAG UPDATE =- */
    if(m_pLocalPlayer->stats.hasFlag)
        m_Map.SetFlagPosition(m_pLocalPlayer->st_PlayerMovementInfo.v_fPos, m_pLocalPlayer->stats.team == 2);

    /* -= MOUSE UPDATE =- */
    static sf::Vector2i prevMousePos = { 0,0 };
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    if (prevMousePos != mousePos || m_bShouldSendPacket) {
        m_pLocalPlayer->st_PlayerMovementInfo.rotation = Utils::r2d(atan2(mousePos.y - (m_pLocalPlayer->playerShape.getPosition().y * ((float)window.getSize().y / g_CamSizeY)),
                                                                          mousePos.x - (m_pLocalPlayer->playerShape.getPosition().x * ((float)window.getSize().x / g_CamSizeX))));
        m_pLocalPlayer->gunShape.setRotation(m_pLocalPlayer->st_PlayerMovementInfo.rotation);
        m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::rotate;
        prevMousePos = mousePos;
        m_bShouldSendPacket = true;
    }

    UpdateLocalPlayerActions();
    
    // TODO rethink bullet system
    /*if (m_coolDown.getElapsedTime().asSeconds() >= m_fFireRate && sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        m_pLocalPlayer->bulletStyle.setPosition(m_pLocalPlayer->playerShape.getPosition() +
            sf::Vector2f(cos(Utils::d2r(m_pLocalPlayer->st_PlayerInfo.rotation)) * m_pLocalPlayer->playerShape.getRadius() * 2.f,
                         sin(Utils::d2r(m_pLocalPlayer->st_PlayerInfo.rotation)) * m_pLocalPlayer->playerShape.getRadius() * 2.f));

        m_pLocalPlayer->bulletStyle.setRotation(m_pLocalPlayer->st_PlayerInfo.rotation);
        m_pLocalPlayer->bullets.push_back(m_pLocalPlayer->bulletStyle);
        m_pLocalPlayer->st_PlayerInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_bullet;
        m_coolDown.restart();
        m_bShouldSendPacket = true;
    }*/
}

void Players::UpdateOtherPlayers()
{
    for (auto& [id, player] : m_mapPlayers) {
        UpdatePlayerBeams(player);
        if (id == m_LocalPlayerID) continue;
        UpdateStatText(player);
        if(m_bInterpolate)
            MovePlayerToPointFixTime(player, player.st_PlayerMovementInfo.v_fPos);
        else
            SetPlayerPos(player, player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y);   //no interpolation
        //MovePlayerToPointFixSpeed(player, player.st_PlayerInfo.v_fPos);
        player.gunShape.setRotation(player.st_PlayerMovementInfo.rotation);

        if(m_pLocalPlayer->st_PlayerActionsInfo.u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) //debug
            realPlayerGhost.setPosition(player.playerShape.getPosition());

        if (player.stats.hasFlag)
            m_Map.SetFlagPosition({ player.playerShape.getPosition().x, player.playerShape.getPosition().y }, player.stats.team == 2);
    }
}

void Players::UpdateObjects()
{
    for (auto& [id, player] : m_mapPlayers) {
        int i = 0;
        for (auto& bullet : player.bullets) {
            bullet.move(cos(Utils::d2r(bullet.getRotation())) * m_fBulletSpeed * m_fDeltaTime, sin(Utils::d2r(bullet.getRotation())) * m_fBulletSpeed * m_fDeltaTime);
            //std::cout << "bpos x: " << bullet.getPosition().x << "  y: " << bullet.getPosition().y << '\n';
            if (bullet.getPosition().x <= m_MapBB.left || bullet.getPosition().y <= m_MapBB.top ||
                bullet.getPosition().x >= m_MapBB.right || bullet.getPosition().y >= m_MapBB.buttom) {
                player.bullets.erase(player.bullets.begin() + i);
            }
            ++i;
        }
    }


    if (m_bShouldCheckBulletCollision) {
        for (auto& [id, player] : m_mapPlayers) {
            for (auto& [id, otherPlayer] : m_mapPlayers)
                for (size_t i = 0; i < player.bullets.size(); i++) {
                    auto& bullet = player.bullets.at(i);
                    if (Utils::CheckCircleCollide(otherPlayer.playerShape.getPosition().x, otherPlayer.playerShape.getPosition().y,
                        otherPlayer.playerShape.getRadius(), bullet.getPosition().x, bullet.getPosition().y, bullet.getRadius())) {
                        std::cout << "coll\n";
                        player.bullets.erase(player.bullets.begin() + i);
                    }
                }
        }
        m_bShouldCheckBulletCollision = false;
    }
}

void Players::UpdateStatText(PlayerStruct& player)
{
    std::stringstream ss;
    ss << player.stats.displayName << '\n';
    ss << "ID: " << player.st_PlayerMovementInfo.n_uID << '\n';
    ss << "PING: " << (int)(player.stats.ping * 1000.f) << "ms" << '\n';
    ss << "HP: " << player.stats.health;
    
    player.statsText.setString(ss.str());
    player.statsText.setPosition(player.playerShape.getPosition());
}

void Players::UpdateLocalPlayerActions()
{
    if (m_pLocalPlayer->stats.ammo > 0 && m_coolDown.getElapsedTime().asSeconds() >= g_fPlayerReloadTime && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        m_pLocalPlayer->beamBlueprint.setPosition(m_pLocalPlayer->playerShape.getPosition());
        m_pLocalPlayer->beamBlueprint.setRotation(m_pLocalPlayer->st_PlayerMovementInfo.rotation);

        static Vector2f rayStart{ 0.f, 0.f };
        static Vector2f rayEnd{ 0.f, 0.f };
        static Vector2f intersectionPoint{ 0.f, 0.f };

        rayStart.x = m_pLocalPlayer->beamBlueprint.getPosition().x;
        rayStart.y = m_pLocalPlayer->beamBlueprint.getPosition().y;

        rayEnd.x = m_pLocalPlayer->beamBlueprint.getPosition().x + cos(Utils::d2r(m_pLocalPlayer->st_PlayerMovementInfo.rotation)) * g_RayLength;
        rayEnd.y = m_pLocalPlayer->beamBlueprint.getPosition().y + sin(Utils::d2r(m_pLocalPlayer->st_PlayerMovementInfo.rotation)) * g_RayLength;

        rayEnd = m_Map.GetRayIntersectionPoint(rayStart, rayEnd);
        m_pLocalPlayer->beamBlueprint.setSize(sf::Vector2f(std::sqrt(std::pow(rayEnd.x - rayStart.x, 2) + std::pow(rayEnd.y - rayStart.y, 2)), m_pLocalPlayer->beamBlueprint.getSize().y));

        for (auto& [id, player] : m_mapPlayers) {
            if (id == m_LocalPlayerID) continue;
            if (Utils::LineCircleCollide(rayStart.x, rayStart.y, rayEnd.x, rayEnd.y,
                player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y, player.playerShape.getRadius(), intersectionPoint))
                hitMarker.setPosition(intersectionPoint.x, intersectionPoint.y);
        }

        m_pLocalPlayer->st_PlayerActionsInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::shot_ray;
        m_coolDown.restart();
        m_bShouldSendPacket = true;
        --m_pLocalPlayer->stats.ammo;

        m_pLocalPlayer->beams.push_back(std::make_pair(m_pLocalPlayer->beamBlueprint, m_clTime.getElapsedTime()));

        m_pLocalPlayer->st_PlayerActionsInfo.playerActionTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        m_pLocalPlayer->st_PlayerActionsInfo.v_fPos = m_pLocalPlayer->st_PlayerMovementInfo.v_fPos;
        m_pLocalPlayer->st_PlayerActionsInfo.rotation = m_pLocalPlayer->st_PlayerMovementInfo.rotation;
#ifdef DEBUG
        printf("Interpolation on server side sould be %f\n", m_pLocalPlayer->st_PlayerActionsInfo.playerActionTime - m_pLocalPlayer->st_PlayerActionsInfo.serverPacketArriveTime);
        printf("playerActionTime %f\n", m_pLocalPlayer->st_PlayerActionsInfo.playerActionTime);
        printf("serverPacketArriveTime %f\n", m_pLocalPlayer->st_PlayerActionsInfo.serverPacketArriveTime);
#endif
        AudioManager::Get().PlayGameSound(GameSounds::PLAYER_SHOOT, 90.f);
    }
}

void Players::UpdatePlayerBeams(PlayerStruct& player)
{
    uint8_t popAmount = 0;
    for (auto& [beam, time] : player.beams) {
        if (m_clTime.getElapsedTime().asSeconds() > time.asSeconds() + m_fBeamDuration + m_fBeamFadeTime)
            ++popAmount;
        else if (m_clTime.getElapsedTime().asSeconds() > time.asSeconds() + m_fBeamDuration) {
            beam.setFillColor(sf::Color(255, 255, 255, beam.getFillColor().a - (sf::Uint8)((255.f / m_fBeamFadeTime) * m_fDeltaTime))); //first try :D
           
        }
    }

    while (popAmount)
        player.beams.pop_front(), --popAmount;
}

struct PlayerStruct;
void Players::SetPlayerPos(uint32_t id, sf::Vector2f pos) {
    PlayerStruct& ps = m_mapPlayers[id];
    ps.playerShape.setPosition(pos);
    ps.gunShape.setPosition(pos);
}
void Players::SetPlayerPos(uint32_t id, float x, float y) {
    PlayerStruct& ps = m_mapPlayers[id];
    ps.playerShape.setPosition(x, y);
    ps.gunShape.setPosition(x, y);
}
void Players::SetPlayerPos(PlayerStruct& player, float x, float y) {
    player.playerShape.setPosition(x, y);
    player.gunShape.setPosition(x, y);
}
void Players::MovePlayer(uint32_t id, sf::Vector2f amount) {
    PlayerStruct& ps = m_mapPlayers[id];
    ps.playerShape.move(amount);
    ps.gunShape.move(amount);
}
void Players::MovePlayer(uint32_t id, float x, float y) {
    PlayerStruct& ps = m_mapPlayers[id];
    ps.playerShape.move(x, y);
    ps.gunShape.move(x, y);
}
void Players::MovePlayer(PlayerStruct& player, float x, float y) {
    player.playerShape.move(x, y);
    player.gunShape.move(x, y);
}

void Players::MovePlayerToPointFixTime(PlayerStruct& player, const Vector2f& targetPosition) {
    // Calculate the distance between the current position and the target position
    static float timePeriod = 1.f / clientUpdateRate; // ex. 100ms time period   (1s / server tick rate)
    float dx = targetPosition.x - player.playerShape.getPosition().x;
    float dy = targetPosition.y - player.playerShape.getPosition().y;
    float distance = sqrt(dx * dx + dy * dy);
    if (distance <= 0.f) return;

    // Calculate the direction vector
    float dirX = dx / distance;
    float dirY = dy / distance;

    // Calculate the required speed based on the distance and time period
    static float requiredSpeed = 0.f;
    if (m_bRecalcInterpolationSpeed) {
        requiredSpeed = distance / timePeriod;
        m_bRecalcInterpolationSpeed = false;
    }

    // Calculate the movement vector
    float moveX = dirX * requiredSpeed * m_fDeltaTime;
    float moveY = dirY * requiredSpeed * m_fDeltaTime;

    // If the object is moving diagonally, adjust the movement vector to maintain a constant speed
   /* if ((fabs(dx) > 0.f && fabs(dy) > 0.f)) {
        moveX *= sqrt(2);
        moveY *= sqrt(2);
    }*/

    // Update the current position
    if (distance <= requiredSpeed * m_fDeltaTime)
        SetPlayerPos(player, targetPosition.x, targetPosition.y);
    else
        MovePlayer(player, moveX, moveY);
}

void Players::MovePlayerToPointFixSpeed(PlayerStruct& player, const Vector2f& targetPosition) {
    // Calculate the distance between the current position and the target position
    float dx = targetPosition.x - player.playerShape.getPosition().x;
    float dy = targetPosition.y - player.playerShape.getPosition().y;
    float distance = sqrt(dx * dx + dy * dy);
    if (distance <= 0.f) return;

    // Calculate the direction vector
    float dirX = dx / distance;
    float dirY = dy / distance;

    // Calculate the movement vector
    float moveX = dirX * m_fSpeed * m_fDeltaTime;
    float moveY = dirY * m_fSpeed * m_fDeltaTime;

    // If the object is moving diagonally, adjust the movement vector to maintain a constant speed
    if ((fabs(dx) > 0 && fabs(dy) > 0)) {
        moveX *= sqrt(2);
        moveY *= sqrt(2);
    }

    // Update the current position
    if (distance <= m_fSpeed * m_fDeltaTime)
        SetPlayerPos(player, player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y);
    else
        MovePlayer(player, moveX, moveY);
}

void Players::UpdateCheats() {
    BotMove();
    if (!window.hasFocus() || Chat::Get().IsActive()) return;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)) {
        TeleportHack();
    } 
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
        DeltaTimeManipulationSpeedHack();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::V)) {
        RapidFireHack();
    }
}

void Players::TeleportHack() {
    std::cout << "Preform tp hack\n";
    MovePlayer(m_LocalPlayerID, 100.f, 0.f);
    auto& locPPos = m_pLocalPlayer->playerShape.getPosition();
    m_bShouldSendPacket = true;
}

void Players::DeltaTimeManipulationSpeedHack() {
    m_fDeltaTime = 1.f / 30.f;
}

void Players::RapidFireHack()
{
    //m_fFireRate = 0.05f;
}

void Players::BotMove() {
    static bool down = true;
    static bool up = false;
    static bool enabled = false;
    static sf::Vector2f startPos;

    if (window.hasFocus() && GetAsyncKeyState('Q') & 1) {
        enabled = !enabled;
        startPos = m_pLocalPlayer->gunShape.getPosition();
    }
    if (!enabled) return;


    if (down && startPos.y + 500 <= m_pLocalPlayer->playerShape.getPosition().y)
        up = true, down = false;
    if (up && startPos.y >= m_pLocalPlayer->playerShape.getPosition().y)
        down = true, up = false;

    if (down)
        MovePlayer(m_LocalPlayerID, 0.f, m_fSpeed * m_fDeltaTime), m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_down;
    if (up)
        MovePlayer(m_LocalPlayerID, 0.f, -m_fSpeed * m_fDeltaTime), m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_up;

    auto& locPPos = m_pLocalPlayer->playerShape.getPosition();
    m_pLocalPlayer->st_PlayerMovementInfo.v_fPos = { locPPos.x, locPPos.y };

    m_bShouldSendPacket = true;
}

void Players::InitPlayer(AddPlayerData& addData)
{
    PlayerStruct ps;
    ps.st_PlayerMovementInfo.v_fPos = addData.v_fPos;
    ps.st_PlayerMovementInfo.n_uID = addData.n_uID;
    std::cout << "Player added with ID: " << addData.n_uID << "\n";

    ps.playerShape.setRadius(m_fPlayerRadius);
    ps.playerShape.setFillColor(sf::Color::Green);
    ps.playerShape.setOrigin(ps.playerShape.getRadius(), ps.playerShape.getRadius());
    ps.playerShape.setPosition(ps.st_PlayerMovementInfo.v_fPos.x, ps.st_PlayerMovementInfo.v_fPos.y);

    ps.gunShape.setSize({ 100.f , 20.f });
    ps.gunShape.setFillColor(sf::Color(100, 100, 100, 255));
    ps.gunShape.setOrigin(0.f, ps.gunShape.getSize().y / 2.f);
    ps.gunShape.setPosition(ps.playerShape.getPosition());

    ps.statsText.setFont(m_Font);
    ps.statsText.setCharacterSize(30);
    if(m_pLocalPlayer != nullptr)
        ps.statsText.setFillColor(ps.stats.team == m_pLocalPlayer->stats.team ? sf::Color(7, 227, 73, 255) : sf::Color(255, 70, 46, 255));
    ps.statsText.setString(std::to_string(addData.n_uID));
    ps.statsText.setOrigin(sf::Vector2f{ ps.statsText.getGlobalBounds().width, ps.statsText.getGlobalBounds().height - 50.f } / 2.f);
    ps.statsText.setPosition(ps.playerShape.getPosition().x, ps.playerShape.getGlobalBounds().top + ps.playerShape.getGlobalBounds().height);

    ps.bulletStyle.setRadius(10.f);
    ps.bulletStyle.setFillColor(sf::Color(100, 100, 255, 255));
    ps.bulletStyle.setOrigin(ps.bulletStyle.getRadius(), ps.bulletStyle.getRadius());
    ps.bulletStyle.setPosition(ps.playerShape.getPosition());

    ps.beamBlueprint.setSize(sf::Vector2f(1920.f, 4.f));
    ps.beamBlueprint.setOrigin(0.f, 2.f);

    ps.stats.score = addData.score;
    ps.stats.team = addData.team;
    ps.stats.hasFlag = addData.b_HasFlag;
    ps.stats.rank = (shared::PlayerRank)addData.rank;
    ps.stats.health = 100;
    ps.stats.ammo = 30;
    ps.stats.ID = addData.n_uID;
    ps.stats.username.assign(addData.sz_Unsername);
    ps.stats.displayName.assign(shared::RankNames[(size_t)addData.rank]);
    ps.stats.displayName.append(addData.sz_Unsername);

    if(!addData.b_WasInGame)
        AudioManager::Get().PlayGameSound(addData.rank == 0 ? GameSounds::VIP_JOIN : GameSounds::PLAYER_JOIN, 80.f);

    m_mapPlayers[addData.n_uID] = ps;

    LeaderBoard::Get().AddNewItem(&m_mapPlayers[addData.n_uID].stats);
    LogSystem::Get().PrintMessage(ps.stats.displayName.c_str(), sf::Color(235, 5, 154, 200));
    LogSystem::Get().PrintMessage(" joined the game.", sf::Color(214, 11, 85, 200), true);
   
}

void Players::AddLocalPlayer(AddPlayerData& addData)
{
     m_pLocalPlayer = &m_mapPlayers[m_LocalPlayerID];

     for (auto& [id, player] : m_mapPlayers)
     {
         player.statsText.setFillColor(player.stats.team == m_pLocalPlayer->stats.team ? sf::Color(7, 227, 73, 255) : sf::Color(255, 70, 46, 255));
     }
}

void Players::RemovePlayer(uint32_t id)
{
    LeaderBoard::Get().RemoveItem(&m_mapPlayers[id].stats);
    LeaderBoard::Get().UpdateText(&m_mapPlayers[id].stats);
    LogSystem::Get().PrintMessage(m_mapPlayers[id].stats.displayName.c_str(), sf::Color(235, 5, 154, 200));
    LogSystem::Get().PrintMessage(" has left the game.", sf::Color(156, 12, 48, 200), true);
    AudioManager::Get().PlayGameSound(GameSounds::PLAYER_LEFT, 90.f);
    m_mapPlayers.erase(id);
    std::cout << "Player removed with ID: " << id << "\n";
    if (id == m_LocalPlayerID) {
        //GameOver();
    }
}

void Players::RegulateLocalPlayer(PlayerMovementData& playerInfo)
{
    SetPlayerPos(m_LocalPlayerID, playerInfo.v_fPos.x, playerInfo.v_fPos.y);
    m_pLocalPlayer->st_PlayerMovementInfo.v_fPos = playerInfo.v_fPos;
    std::cout << "local player regulated\n";
}

void Players::ProcessOtherPlayerMovement(PlayerMovementData& playerInfo)
{
    m_bRecalcInterpolationSpeed = true;
    auto& player = m_mapPlayers[playerInfo.n_uID]; //other players
    player.st_PlayerMovementInfo = playerInfo;

    m_pLocalPlayer->st_PlayerActionsInfo.serverPacketArriveTime = m_DeltaTimeTimePoint;
    m_pLocalPlayer->st_PlayerActionsInfo.lastMovementPacketArriveTime = playerInfo.sv_PacketArriveTime;
    serverUpdatedPlayerPos.setPosition(playerInfo.v_fPos.x, playerInfo.v_fPos.y);

    player.st_PlayerActionsInfo.u_PlayerActions = 0;
}

void Players::ProcessOtherPlayerActions(PlayerActionsData& playerInfo)
{
    auto& player = m_mapPlayers[playerInfo.n_uID]; //other players
    player.st_PlayerActionsInfo = playerInfo;

    if (playerInfo.u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) {
        //we dont really need to verify the shot client side ig...
        /*if (m_pLocalPlayer->beam.getPosition() != sf::Vector2f{ player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y }
            || m_pLocalPlayer->beam.getRotation() != player.st_PlayerMovementInfo.rotation)
            printf("\nShot misscalculated by someone...");*/

        if (playerInfo.n_uID != m_LocalPlayerID) {
            static Vector2f rayStart{ 0.f, 0.f };
            static Vector2f rayEnd{ 0.f, 0.f };

            rayStart = playerInfo.v_fPos;
            rayEnd.x = playerInfo.v_fPos.x + cos(Utils::d2r(playerInfo.rotation)) * g_RayLength;
            rayEnd.y = playerInfo.v_fPos.y + sin(Utils::d2r(playerInfo.rotation)) * g_RayLength;

            rayEnd = m_Map.GetRayIntersectionPoint(rayStart, rayEnd);
            player.beamBlueprint.setSize(sf::Vector2f(std::sqrt(std::pow(rayEnd.x - rayStart.x, 2) + std::pow(rayEnd.y - rayStart.y, 2)), player.beamBlueprint.getSize().y));

            player.beamBlueprint.setPosition(playerInfo.v_fPos.x, playerInfo.v_fPos.y);
            player.beamBlueprint.setRotation(playerInfo.rotation);
            player.beams.push_back(std::make_pair(player.beamBlueprint, m_clTime.getElapsedTime()));
            AudioManager::Get().PlayGameSound(GameSounds::PLAYER_SHOOT, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos, player.st_PlayerMovementInfo.v_fPos);
        }
    }

    if (playerInfo.u_PlayerActions & (UINT32)PLAYER_ACTION::shot_bullet) {
        player.bulletStyle.setPosition(sf::Vector2f{ player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y } +
            sf::Vector2f(cos(Utils::d2r(player.st_PlayerMovementInfo.rotation)) * player.playerShape.getRadius() * 2.f, sin(Utils::d2r(player.st_PlayerMovementInfo.rotation)) * player.playerShape.getRadius() * 2.f));
        player.bulletStyle.setRotation(player.st_PlayerMovementInfo.rotation);
        player.bullets.push_back(player.bulletStyle);
    }

    player.st_PlayerActionsInfo.u_PlayerActions = 0;
}

void Players::UpdateGameStateVariables(sv_HitregData& hitregInfo)
{
    auto& player = m_mapPlayers[hitregInfo.n_uID];
    auto& perpetrator = m_mapPlayers[hitregInfo.n_uPerpetratorID];

    if (hitregInfo.u_HitregFlag & (UINT32)FLAGS_HITREG::player_hit) {
        m_bShouldCheckBulletCollision = true;
        std::cout << "player hit ball\n";
    }
    if (hitregInfo.u_HitregFlag & (UINT32)FLAGS_HITREG::player_hit_ray) {
        std::cout << "player hit ray\n";
        lagCompensatedPlayerGhost.setPosition(hitregInfo.v_fPos.x, hitregInfo.v_fPos.y);
        realLocalPlayerGhost.setPosition(m_pLocalPlayer->playerShape.getPosition());
        player.stats.health -= hitregInfo.u_Damage;
        if (hitregInfo.n_uPerpetratorID == m_LocalPlayerID)
            AudioManager::Get().PlayGameSound(GameSounds::HIT_FEEDBACK, 100.f);
        AudioManager::Get().PlayGameSound(GameSounds::PLAYER_HIT, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos, hitregInfo.v_fPos, 70.f);
    }
}

void Players::HandlePlayerKill(sv_PLayerDiedData& playerStateinfo)
{
    auto& killedPlayer = m_mapPlayers[playerStateinfo.n_uID];
    auto& killerPlayer = m_mapPlayers[playerStateinfo.n_uPerpetratorID];

    printf("Player [%s] killed by [%s]\n", killedPlayer.stats.username.c_str(), killerPlayer.stats.username.c_str());
    Killfeed::Get().AppendKillfeed(m_pLocalPlayer, &killerPlayer, &killedPlayer);

    killerPlayer.stats.score += g_KillReward_score;
    LeaderBoard::Get().SortItems();
    LeaderBoard::Get().UpdateText(&killerPlayer.stats);

    SetPlayerPos(playerStateinfo.n_uID, playerStateinfo.v_fRespawnPos.x, playerStateinfo.v_fRespawnPos.y);
    killedPlayer.st_PlayerMovementInfo.v_fPos = playerStateinfo.v_fRespawnPos;
    killedPlayer.stats.hasFlag = false;
    killedPlayer.stats.health = 100;
    killedPlayer.stats.ammo = 30;
    killedPlayer.stats.hasFlag = false;

    AudioManager::Get().PlayGameSound(GameSounds::PLAYER_DIED, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos, killedPlayer.st_PlayerMovementInfo.v_fPos);
    if (playerStateinfo.n_uPerpetratorID == m_LocalPlayerID) {
        m_pLocalPlayer->stats.ammo += g_KillReward_ammo;
        AudioManager::Get().PlayGameSound(GameSounds::TANK_KILLED_FEEDBACK, 100.f);
    }
}

void Players::HandleStatusUdpate(shared::UserCommand* usercmd)
{
    PlayerStruct& affectedPlayer = m_mapPlayers[usercmd->uIDRight];

    //Handle rank change
    if (usercmd->uUserCommand == shared::UserCmd::SET_RANK)
    {
        const char* rankName = shared::RankNames[usercmd->iValue];
        affectedPlayer.stats.displayName = rankName + affectedPlayer.stats.username;
        LeaderBoard::Get().UpdateText(&affectedPlayer.stats);
        if (affectedPlayer.stats.ID == m_LocalPlayerID) {
            Chat::Get().AppendPlainMessage(sf::Color(24, 205, 237, 255), "You'r rank has changed to %s", rankName);
            switch ((shared::PlayerRank)usercmd->iValue)
            {
            case shared::PlayerRank::OP:
                Chat::Get().AppendPlainMessage(sf::Color(24, 205, 237, 255), "-={ You Are Now Op! }=-"); break;
            default:
                if (affectedPlayer.stats.rank == shared::PlayerRank::OP)
                    Chat::Get().AppendPlainMessage(sf::Color(24, 205, 237, 255), "-={ You are no longer op. }=-"); break;
            }
            g_bNeedUiUpdate = true;
        }
        affectedPlayer.stats.rank = (shared::PlayerRank)usercmd->iValue;
        return;
    }

    switch (usercmd->uUserCommand)
    {
    case shared::UserCmd::ADD_AMMO:
    {
        m_pLocalPlayer->stats.ammo += usercmd->iValue;
        Chat::Get().AppendPlainMessage(sf::Color(24, 205, 237, 255), ('[' + m_mapPlayers[usercmd->uIDLeft].stats.displayName + "] added ammo: " + std::to_string(usercmd->iValue)).c_str());
    } break;
    case shared::UserCmd::ADD_HEALTH:
    {
        if (usercmd->uIDRight == m_LocalPlayerID) {
            m_pLocalPlayer->stats.health += usercmd->iValue;
            Chat::Get().AppendPlainMessage(sf::Color(24, 205, 237, 255), ('[' + m_mapPlayers[usercmd->uIDLeft].stats.displayName + "] added health: " + std::to_string(usercmd->iValue)).c_str());
        }
        else {
            affectedPlayer.stats.health += usercmd->iValue;
        }
    } break;

    default: break;
    }
}

void Players::HandleFlagStateChange(MapFlagStatusUpdateData* flagData)
{
    PlayerStruct& perpetrator = m_mapPlayers[flagData->n_uID];
    perpetrator.stats.hasFlag = flagData->flagState == (int8_t)shared::FlagStates::FLAG_STOLEN
    || perpetrator.stats.hasFlag && flagData->flagState == (int8_t)shared::FlagStates::FLAG_RETURNED;

    switch ((shared::FlagStates)flagData->flagState)
    {
    case shared::FlagStates::FLAG_STOLEN:
        AudioManager::Get().PlayGameSound(GameSounds::FLAG_STOLE, 100.f);
        break;
    case shared::FlagStates::FLAG_RETURNED:
        AudioManager::Get().PlayGameSound(GameSounds::FLAG_RETURN, 100.f);
        m_Map.SetFlagPosition(perpetrator.stats.team == 1 ? shared::FlagStandPosLeft : shared::FlagStandPosRight, perpetrator.stats.team == 1);    //fuck you
        break;
    case shared::FlagStates::FLAG_WIN:
        AudioManager::Get().PlayGameSound(GameSounds::FLAG_WIN, 100.f);
        m_Map.SetFlagPosition(perpetrator.stats.team == 1 ? shared::FlagStandPosRight : shared::FlagStandPosLeft, perpetrator.stats.team == 2);

        perpetrator.stats.team == 1 ? ++g_TeamFlagScore : ++g_EnemyFlagScore;
        g_bNeedUiUpdate = true;
        break;
    case shared::FlagStates::FLAG_DROPPED:
        m_Map.SetFlagPosition(flagData->v_fPos, perpetrator.stats.team == 2);
        AudioManager::Get().PlayGameSound(GameSounds::FLAG_LOST, 80.f);
        break;
    case shared::FlagStates::FLAG_LOST:
        m_Map.SetFlagPosition(flagData->v_fPos, perpetrator.stats.team == 2);
        AudioManager::Get().PlayGameSound(GameSounds::FLAG_LOST, 100.f);
        break;
    default:
        break;
    }
}

void Players::CalcPing(const PlayerMovementData& playerInfo, float RTT)
{
    static PlayerStruct* pp = nullptr;
    pp = &m_mapPlayers[playerInfo.n_uID];
    if (pp->lastPingUpdate < m_clTime.getElapsedTime().asSeconds()) {
        pp->stats.ping = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - RTT;
        pp->lastPingUpdate = pp->stats.ping + 1.f;
    }
}

void Players::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
    m_Map.drawFlags(target);

    for (auto& player : m_mapPlayers) { //cpp 17 [k,v] mb cleaner
        target.draw(player.second.playerShape);
        for (auto& bullet : player.second.bullets)
            target.draw(bullet);
        for(auto& beam : player.second.beams)
            target.draw(beam.first);
        target.draw(player.second.gunShape);
        if (player.first != m_LocalPlayerID)
            target.draw(player.second.statsText);

    }
    target.draw(m_Map);
    target.draw(realPlayerGhost);
    target.draw(realLocalPlayerGhost);
    target.draw(lagCompensatedPlayerGhost);
    target.draw(serverUpdatedPlayerPos);
    target.draw(hitMarker);
    //target.draw(cs);
}

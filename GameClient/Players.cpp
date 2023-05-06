#include "Players.hpp"
#include "SharedVariables.hpp"
#include "LogSystem.hpp"
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
        PlayerMovementData movementInfo;
        movementInfo.n_uID = 1000;
        movementInfo.v_fPos = { 100.f ,100.f };
        InitPlayer(movementInfo);
        AddLocalPlayer(movementInfo);
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

void Players::UpdateLocalPlayer()
{
    UpdateCheats();
    if (!window.hasFocus()) return;


    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.x -= m_fSpeed * m_fDeltaTime, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_left;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.x += m_fSpeed * m_fDeltaTime, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_right;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.y -= m_fSpeed * m_fDeltaTime, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_up;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.y += m_fSpeed * m_fDeltaTime, m_pLocalPlayer->st_PlayerMovementInfo.u_PlayerActions |= (UINT32)PLAYER_ACTION::move_down;

    m_Map.ArrangePlayerCollision(m_pLocalPlayer->st_PlayerMovementInfo.v_fPos);

    SetPlayerPos(m_LocalPlayerID, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.x, m_pLocalPlayer->st_PlayerMovementInfo.v_fPos.y);

    static sf::Vector2i prevMousePos = { 0,0 };
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    if (prevMousePos != mousePos || m_bShouldSendPacket) {
        m_pLocalPlayer->gunShape.setRotation(Utils::r2d(atan2(mousePos.y - m_mapPlayers[m_LocalPlayerID].playerShape.getPosition().y, mousePos.x - m_mapPlayers[m_LocalPlayerID].playerShape.getPosition().x)));
        m_pLocalPlayer->st_PlayerMovementInfo.rotation = m_pLocalPlayer->gunShape.getRotation();
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
        UpdateStatText(player);
        UpdatePlayerBeams(player);
        if (id == m_LocalPlayerID) continue;
        if(m_bInterpolate)
            MovePlayerToPointFixTime(player, player.st_PlayerMovementInfo.v_fPos);
        else
            SetPlayerPos(player, player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y);   //no interpolation
        //MovePlayerToPointFixSpeed(player, player.st_PlayerInfo.v_fPos);
        player.gunShape.setRotation(player.st_PlayerMovementInfo.rotation);

        if(m_pLocalPlayer->st_PlayerActionsInfo.u_PlayerActions & (UINT32)PLAYER_ACTION::shot_ray) //debug
            realPlayerGhost.setPosition(player.playerShape.getPosition());
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
    ss << "ID: " << player.st_PlayerMovementInfo.n_uID << '\n';
    ss << "PING: " << (int)(player.ping * 1000.f) << "ms" << '\n';
    ss << "HP: " << player.health;
    
    player.statsText.setString(ss.str());
    player.statsText.setPosition(player.playerShape.getPosition());
}

void Players::UpdateLocalPlayerActions()
{
    if (m_coolDown.getElapsedTime().asSeconds() >= m_fFireRate && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
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

        m_pLocalPlayer->beams.push_back(std::make_pair(m_pLocalPlayer->beamBlueprint, m_clTime.getElapsedTime()));

        m_pLocalPlayer->st_PlayerActionsInfo.playerActionTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#ifdef DEBUG
        printf("Interpolation on server side sould be %f\n", m_pLocalPlayer->st_PlayerActionsInfo.playerActionTime - m_pLocalPlayer->st_PlayerActionsInfo.serverPacketArriveTime);
        printf("playerActionTime %f\n", m_pLocalPlayer->st_PlayerActionsInfo.playerActionTime);
        printf("serverPacketArriveTime %f\n", m_pLocalPlayer->st_PlayerActionsInfo.serverPacketArriveTime);
#endif
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
    if (!window.hasFocus()) return;
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
    m_fFireRate = 0.05f;
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

void Players::InitPlayer(PlayerMovementData& info)
{
    PlayerStruct ps;
    ps.st_PlayerMovementInfo = info;

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
    ps.statsText.setString(std::to_string(info.n_uID));
    ps.statsText.setOrigin(sf::Vector2f{ ps.statsText.getGlobalBounds().width, ps.statsText.getGlobalBounds().height - 50.f } / 2.f);
    ps.statsText.setFillColor(sf::Color::Red);
    ps.statsText.setPosition(ps.playerShape.getPosition().x, ps.playerShape.getGlobalBounds().top + ps.playerShape.getGlobalBounds().height);

    ps.bulletStyle.setRadius(10.f);
    ps.bulletStyle.setFillColor(sf::Color(100, 100, 255, 255));
    ps.bulletStyle.setOrigin(ps.bulletStyle.getRadius(), ps.bulletStyle.getRadius());
    ps.bulletStyle.setPosition(ps.playerShape.getPosition());

    ps.beamBlueprint.setSize(sf::Vector2f(1920.f, 4.f));
    ps.beamBlueprint.setOrigin(0.f, 2.f);

    ps.health = 100;

    m_mapPlayers[info.n_uID] = ps;
}

void Players::AddLocalPlayer(PlayerMovementData& playerInfo)
{
    std::cout << "Player added with ID: " << playerInfo.n_uID << "\n";
    m_pLocalPlayer = &m_mapPlayers[m_LocalPlayerID];
}

void Players::RemovePlayer(uint32_t id)
{
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

            rayStart.x = player.st_PlayerMovementInfo.v_fPos.x;
            rayStart.y = player.st_PlayerMovementInfo.v_fPos.y;
            rayEnd.x = player.st_PlayerMovementInfo.v_fPos.x + cos(Utils::d2r(player.st_PlayerMovementInfo.rotation)) * g_RayLength;
            rayEnd.y = player.st_PlayerMovementInfo.v_fPos.y + sin(Utils::d2r(player.st_PlayerMovementInfo.rotation)) * g_RayLength;

            rayEnd = m_Map.GetRayIntersectionPoint(rayStart, rayEnd);
            player.beamBlueprint.setSize(sf::Vector2f(std::sqrt(std::pow(rayEnd.x - rayStart.x, 2) + std::pow(rayEnd.y - rayStart.y, 2)), player.beamBlueprint.getSize().y));

            player.beamBlueprint.setPosition(player.st_PlayerMovementInfo.v_fPos.x, player.st_PlayerMovementInfo.v_fPos.y);
            player.beamBlueprint.setRotation(player.st_PlayerMovementInfo.rotation);
            player.beams.push_back(std::make_pair(player.beamBlueprint, m_clTime.getElapsedTime()));
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

    if (hitregInfo.u_HitregFlag & (UINT32)FLAGS_HITREG::player_hit) {
        m_bShouldCheckBulletCollision = true;
        std::cout << "player hit ball\n";
    }
    if (hitregInfo.u_HitregFlag & (UINT32)FLAGS_HITREG::player_hit_ray) {
        std::cout << "player hit ray\n";
        lagCompensatedPlayerGhost.setPosition(hitregInfo.v_fPos.x, hitregInfo.v_fPos.y);
        realLocalPlayerGhost.setPosition(m_pLocalPlayer->playerShape.getPosition());
        player.health -= hitregInfo.u_Damage;
    }
}

void Players::UpdatePlayerStateVariables(sv_PlayerStateData& playerStateinfo)
{
    auto& player = m_mapPlayers[playerStateinfo.n_uID];

    if (playerStateinfo.u_PlayerStates & (UINT32)PLAYER_STATE::respawn) {
        SetPlayerPos(playerStateinfo.n_uID, playerStateinfo.v_fPos.x, playerStateinfo.v_fPos.y);
        player.st_PlayerMovementInfo.v_fPos = playerStateinfo.v_fPos;
        player.health = 100;
    }
}

void Players::CalcPing(const PlayerMovementData& playerInfo, float RTT)
{
    static PlayerStruct* pp = nullptr;
    pp = &m_mapPlayers[playerInfo.n_uID];
    if (pp->lastPingUpdate < m_clTime.getElapsedTime().asSeconds()) {
        pp->ping = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - RTT;
        pp->lastPingUpdate += 1.f;
    }
}

void Players::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
    for (auto& player : m_mapPlayers) { //cpp 17 [k,v] mb cleaner
        target.draw(player.second.playerShape);
        for (auto& bullet : player.second.bullets)
            target.draw(bullet);
        for(auto& beam : player.second.beams)
            target.draw(beam.first);
        target.draw(player.second.gunShape);
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

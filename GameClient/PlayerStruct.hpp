#pragma once
#include <SFML/Graphics.hpp>
#include "stdIncludes.hpp"
#include "PacketStruct.hpp"

struct PlayerStatsStruct
{
    std::string username;
    std::string displayName;
    float ping;
    int health = 0;
    int score = 0;
    uint32_t ammo;
    uint32_t ID;
    int8_t team = -1;
    bool hasFlag = false;
    shared::PlayerRank rank;
};

struct PlayerStruct
{
    sf::CircleShape playerShape;
    sf::RectangleShape gunShape;
    sf::Text statsText;
    float lastPingUpdate = 0.f;
 
    std::deque<sf::CircleShape> bullets;
    sf::CircleShape bulletStyle;
    std::deque<std::pair<sf::RectangleShape, sf::Time>> beams;
    sf::RectangleShape beamBlueprint;

    PlayerStatsStruct stats;

    PlayerMovementData st_PlayerMovementInfo;
    PlayerActionsData st_PlayerActionsInfo;
    Vector2f dirVec;
};
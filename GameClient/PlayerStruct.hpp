#pragma once
#include <SFML/Graphics.hpp>
#include "stdIncludes.hpp"
#include "PacketStruct.hpp"

struct PlayerStruct
{
    sf::CircleShape playerShape;
    sf::RectangleShape gunShape;
    sf::Text statsText;
    float lastPingUpdate = 0.f;
    float ping;
    int health;
    std::deque<sf::CircleShape> bullets;
    sf::CircleShape bulletStyle;
    std::deque<std::pair<sf::RectangleShape, sf::Time>> beams;
    sf::RectangleShape beamBlueprint;

    PlayerMovementData st_PlayerMovementInfo;
    PlayerActionsData st_PlayerActionsInfo;
};
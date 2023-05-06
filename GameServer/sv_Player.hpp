#pragma once
#include <iostream>
#include <deque>
#include <vector>

#include "PacketStruct.hpp"
#include "utils.hpp"

struct Bullet
{
	Vector2f pos;
	float rotation;
};

struct PlayerRecord
{
	double d_ServerTimePoint;
	double d_ClientTimePoint;
	Vector2f Pos;
};

struct PlayerObjects
{
	std::deque<Bullet> bullets;
	std::vector<PlayerRecord> records;
	bool shotRay = false;
	double lastShotTime = 0.0;
};

struct PacketInfo
{
	double cl_PacketDispatchTime = 0.0;
	double sv_PacketArriveTime = 0.0;
};

struct GameState
{
	PacketInfo packetInfo;
	PlayerMovementData playerMovementData;
	PlayerActionsData playerActionsData;
	PlayerObjects playerObjects;
	Vector2f rayStart;
	Vector2f rayEnd;
	int health;
};

class Player
{
public:
	Player();
	~Player();

private:

};


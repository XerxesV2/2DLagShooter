#pragma once
#include "includes.hpp"
#include "PacketStruct.hpp"

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

struct PlayerInfo
{
	std::string s_Username;
};

struct GameState
{
	PacketInfo packetInfo;
	PlayerInfo playerInfo;
	AddPlayerData playerAddData;
	PlayerMovementData playerMovementData;
	PlayerActionsData playerActionsData;
	PlayerObjects playerObjects;
	Vector2f rayStart;
	Vector2f rayEnd;
	int health;
	int ammo;
	int score;
};
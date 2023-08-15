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
	float rotation;
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
	uint32_t u_id;
	uint32_t u_gid;
	std::string s_Username;
	std::string s_Hwid;
	shared::PlayerRank rank;
	int8_t team = -1;
};

struct PlayerStatistic
{
	int health;
	int ammo;
	int score;
};

struct PlayerStates
{
	bool b_HasFlag = false;
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
	PlayerStatistic stats;
	PlayerStates state;
};
#pragma once
#include "SharedVariables.hpp"

enum class PLAYER_ACTION : unsigned int
{
	move_left		= 1 << 0,
	move_right		= 1 << 1,
	move_up			= 1 << 2,
	move_down		= 1 << 3,
	rotate			= 1 << 4,
	shot_bullet		= 1 << 5,
	shot_ray		= 1 << 6,
	none,
};

enum class PLAYER_STATE : unsigned char
{
	respawn		= 1 << 0,
};

enum class FLAGS_HITREG : unsigned int
{
	player_hit		= 1 << 0,
	player_hit_ray	= 1 << 1,
};

enum class FLAGS_CHATMSG : unsigned char
{
	server_msg		,
	client_msg		,
	private_msg		,
	moderate_msg	,
};

//#pragma pack(push, 1)
struct PlayerMovementData
{
	unsigned int n_uID;
	unsigned int n_uSequence;

	//int n_HP;
	//int n_Ammo;
	//int n_Kills;

	float deltaTime;
	float sv_PacketArriveTime;
	float rotation;
	
	Vector2f v_fPos;
	unsigned int u_PlayerActions{ 0 };
};

struct AddPlayerData
{
	unsigned int n_uID;
	char sz_Unsername[20]{0};
	int score = 0;
	unsigned char rank;
	signed char team = -1;
	bool b_HasFlag = false;
	bool b_WasInGame = false;
	Vector2f v_fPos;
};

struct WorldData
{
	int i_TeamOneScore = 0;
	int i_TeamTwoScore = 0;
	Vector2f v_fTeamOneFlagPos;
	Vector2f v_fTeamTwoFlagPos;
};

struct PlayerActionsData
{
	unsigned int n_uID;

	double serverPacketArriveTime;
	double playerActionTime;	//what a waste
	float lastMovementPacketArriveTime;

	unsigned int u_PlayerActions{ 0 };

	Vector2f v_fPos;
	float rotation;
};

struct ChatMessageData
{
	unsigned int n_uID;
	unsigned char fMsgFlags = 0;
	char msg[maxChatMessageLength] = {0};
};

struct MapFlagStatusUpdateData
{
	unsigned int n_uID;
	signed char flagState;
	Vector2f v_fPos;
};

struct sv_HitregData
{
	unsigned int n_uID;
	unsigned int n_uPerpetratorID;

	unsigned int u_Damage;
	unsigned int u_HitregFlag;
	Vector2f v_fPos;
};

struct sv_PLayerDiedData
{
	unsigned int n_uID;
	unsigned int n_uPerpetratorID;
	Vector2f v_fRespawnPos;
};

//#pragma pack(pop)
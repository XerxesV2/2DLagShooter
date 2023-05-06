#pragma once

struct Vector2f
{
	Vector2f() = default;
	Vector2f(float _x, float _y) : x(_x), y(_y) {}

	float x;
	float y;

	bool operator == (const Vector2f& rhs) {
		return x == rhs.x && y == rhs.y;
	}

	bool operator != (const Vector2f& rhs) {
		return x != rhs.x || y != rhs.y;
	}

	Vector2f operator + (const Vector2f&& rhs) {
		return Vector2f(x + rhs.x, y + rhs.y);
	}
	Vector2f operator + (const Vector2f& rhs) {
		return Vector2f(x + rhs.x, y + rhs.y);
	}

	Vector2f operator - (const Vector2f&& rhs) {
		return Vector2f(x - rhs.x, y - rhs.y);
	}
	Vector2f operator - (const Vector2f& rhs) {
		return Vector2f(x - rhs.x, y - rhs.y);
	}

	Vector2f operator * (const float&& rhs) {
		return Vector2f(x * rhs, y * rhs);
	}
	Vector2f operator * (const float& rhs) {
		return Vector2f(x * rhs, y * rhs);
	}

	void operator += (const Vector2f& rhs) {
		x += rhs.x;
		y += rhs.y;
	}
};

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

struct PlayerActionsData
{
	unsigned int n_uID;

	double serverPacketArriveTime;
	double playerActionTime;	//what a waste
	float lastMovementPacketArriveTime;

	unsigned int u_PlayerActions{ 0 };
};

struct sv_HitregData
{
	unsigned int n_uID;

	unsigned int u_Damage;
	unsigned int u_HitregFlag;
	Vector2f v_fPos;
};

struct sv_PlayerStateData
{
	unsigned int n_uID;

	unsigned int u_PlayerStates{ 0 };
	Vector2f v_fPos;
};
//#pragma pack(pop)
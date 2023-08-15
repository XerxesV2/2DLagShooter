#pragma once
struct Vector2f
{
	Vector2f() : x(0), y(0) {}
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

	Vector2f operator + (const float rhs) {
		return Vector2f(x + rhs, y + rhs);
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

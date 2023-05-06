#pragma once
#include <SFML/Graphics.hpp>

#include "stdIncludes.hpp"
#include "PlayerStruct.hpp"

class GameMap : public sf::Drawable
{
public:
	GameMap();
	~GameMap();

	//Sets the player position as well
	void ArrangePlayerCollision(Vector2f& playerPos);
	//g_RayLength = no collision happened
	[[nodiscard]] Vector2f GetRayIntersectionPoint(Vector2f rayStart, Vector2f rayEnd);

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	void DoPlayerWallCollision(Vector2f& playerPos);
	void DoPlayerMapBorderCollsion(Vector2f& playerPos);

private:
	std::vector<sf::RectangleShape> m_vObjects;
};


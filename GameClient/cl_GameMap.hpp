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
	void SetFlagPosition(Vector2f pos, bool red_blue);
	//g_RayLength = no collision happened
	[[nodiscard]] Vector2f GetRayIntersectionPoint(Vector2f rayStart, Vector2f rayEnd);

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;
	void drawFlags(sf::RenderTarget& target) const;

private:
	void DoPlayerWallCollision(Vector2f& playerPos);
	void DoPlayerMapBorderCollsion(Vector2f& playerPos);

private:
	std::vector<sf::RectangleShape> m_vObjects;

	sf::Texture m_RedFlagTexture;
	sf::Texture m_BlueFlagTexture;
	sf::Sprite m_RedFlagSprite;
	sf::Sprite m_BlueFlagSprite;

	sf::Texture m_RedStandTexture;
	sf::Texture m_BlueStandTexture;
	sf::Sprite m_RedStandSprite;
	sf::Sprite m_BlueStandSprite;
};


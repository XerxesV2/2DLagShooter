#pragma once
#include "GameNetwork.hpp"
#include "GameMap.h"
#include "Players.hpp"

class Game
{
public:
	Game();
	~Game();

    bool Connect();
	void MainLoop();

private:
    void UpdateClient();
    void DrawFrame();
    void HandleEvents();
    void UpdateOptions();

private:
    /* init list */
    sf::RenderWindow window;

    sf::View view;
    std::shared_ptr<GameNetwork> m_pGameNetwork;
    std::shared_ptr<Players> m_pPlayers;

    sf::Clock m_DeltaClock;
    float m_fDeltaTime;

    sf::Font m_Font;
    sf::Text m_ConnectingText;
    sf::Text m_GameOverText;
    sf::Event event;

    bool m_bGameOver = false;
};


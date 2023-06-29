#pragma once
#include "cl_GameNetwork.hpp"
#include "cl_GameMap.hpp"
#include "cl_Players.hpp"
#include "cl_Login.hpp"
#include "UI/UI.hpp"

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
    std::shared_ptr<UI> m_pUI;
    std::unique_ptr<Login> m_pLogin;

    sf::Clock m_DeltaClock;
    float m_fDeltaTime;
    std::deque<uint32_t> m_PressedKeyCodes;

    sf::Font m_Font;
    sf::Text m_ConnectingText;
    sf::Text m_GameOverText;
    sf::Event event;

    bool m_bGameOver = false;
};


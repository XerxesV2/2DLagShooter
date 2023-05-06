#include "Game.hpp"

int main()
{
    Game game;
    game.Connect();
    game.MainLoop();

    return EXIT_SUCCESS;
}
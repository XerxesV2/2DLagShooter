#include "Server.hpp"

const double targetFrameTime = 1.0 / serverTickRate; // x frames per second
double previousTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
double lagTime = 0.0;

GameServer* pgs = nullptr;
BOOL WINAPI HandlerRoutine(
	_In_ DWORD dwCtrlType
)
{
	if (dwCtrlType == CTRL_CLOSE_EVENT)
		pgs->ShutDown();

	return FALSE;
}

int main() 
{
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	GameServer gs(6969);
	pgs = &gs;
	std::cout << "Tick rate: " << serverTickRate << std::endl;
	gs.Start();
	while (1)
	{
		double currentTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		double elapsedTime = currentTime - previousTime;
		previousTime = currentTime;

		lagTime += elapsedTime;

		while (lagTime >= targetFrameTime) {
			gs.Update(100, false);
			gs.UpdateGameState(currentTime);

			g_fDeltaTime = elapsedTime;
			//std::cout << "Delta time: " << g_fDeltaTime << " seconds" << std::endl;

			lagTime -= targetFrameTime;
		}
	}
	gs.ShutDown();

	return EXIT_SUCCESS;
}
#pragma once
#include "json.hpp"
#include "PlayerStats.hpp"

class Database
{
public:
	Database();
	~Database();

	void LoadFromFile();
	void SaveToFile();

	//sets specific variables from the database. return 1 if cant find player in database 0 otherwise
	bool GetPlayerStats(GameState& playerState);
	long long GetBanTime(const char* sz_hwid);
	//updates the the player in the database
	void UpdatePlayerData(const GameState& playerState);
	void BanPlayer(std::string& hwid, long long banTime);

	void AddNewPlayer(uint32_t id, shared::LoginInformation& logininfo);
	void PlayerLeft(std::string& hwid);
	bool IsPlayerNameExists(const char* name);
	bool IsPlayerBanned(shared::LoginInformation& logininfo);
	bool IsPlayerExists(shared::LoginInformation& logininfo);
	bool IsPlayerExists(std::string& hwid);
	bool IsPlayerExists(const char* sz_hwid);
	bool IsPlayerOnline(std::string& hwid);
	bool IsPlayerOnline(const char* sz_hwid);
	shared::PlayerRank GetPlayerRank(const char* sz_hwid);

private:

	struct PlayerData
	{
		//saved data
		char hwid[39];
		char username[20];
		int score = 0;
		shared::PlayerRank rank = shared::PlayerRank::DEFAULT;
		long long banTimeInSec = 0.;
		bool banned = false;
		bool permaBanned = false;

		//other data
		bool online = false;
	};
	std::unordered_map<std::string, PlayerData> m_MapPlayerData;
	std::unordered_map<std::string, std::string> m_MapPlayerNamesAndHwid;

	nlohmann::json m_json;
};


#include "Database.hpp"
#include "utils.hpp"
#include <fstream>

Database::Database()
{
}

Database::~Database()
{
}

void Database::LoadFromFile()
{
	std::time_t llCurrTime = std::time(nullptr);
	try
	{
		std::ifstream is("database.json");
		is >> m_json;

		PlayerData data;
		for (const auto& item : m_json.items())
		{
			strcpy_s(data.hwid, item.value()["hwid"].get<std::string>().c_str());
			strcpy_s(data.username, item.key().c_str());

			data.score			= item.value()["score"].get<int>();
			data.rank			= item.value()["rank"].get<shared::PlayerRank>();
			data.banTimeInSec	= item.value()["banTimeInSec"].get<long long>();
			data.permaBanned	= item.value()["permaBanned"].get<bool>();

			data.banned = (llCurrTime < data.banTimeInSec) || data.permaBanned;

			m_MapPlayerData.insert({ data.hwid, data });
			m_MapPlayerNamesAndHwid.insert({ item.key(), data.hwid });

			//std::cout << item.key() << "\n";
			//std::cout << "  " << item.value()["hwid"].get<std::string>() << "\n";
			//std::cout << "  " << item.value()["score"].get<int>() << "\n";
		}
		is.close();
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
}

void Database::SaveToFile()
{
	try
	{
		for (const auto& [key, data] : m_MapPlayerData)
		{
			m_json[data.username]["hwid"]			= data.hwid;
			m_json[data.username]["score"]			= data.score;
			m_json[data.username]["rank"]			= data.rank;
			m_json[data.username]["banTimeInSec"]	= data.banTimeInSec;
			m_json[data.username]["banned till"]	= Utils::TimeToString(data.banTimeInSec);
			m_json[data.username]["banned"]			= data.banned;
			m_json[data.username]["permaBanned"]	= data.permaBanned;
		}

		std::ofstream os("database.json");
		os << std::setw(4) << m_json << std::endl;
		os.close();
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
}

bool Database::GetPlayerStats(GameState& playerState)
{
	if (m_MapPlayerData.count(playerState.playerInfo.s_Hwid) != 0)
	{
		Database::PlayerData& data = m_MapPlayerData[playerState.playerInfo.s_Hwid];

		playerState.playerInfo.rank			= data.rank;
		playerState.playerInfo.s_Username	= data.username;
		playerState.stats.score				= data.score;

		if (data.online)
			playerState.playerInfo.s_Username.append(std::to_string(std::rand() % 9998 + 1));

		data.online = true;
		return 0;
	}
	return 1;
}

long long Database::GetBanTime(const char* sz_hwid)
{
	return m_MapPlayerData[sz_hwid].banTimeInSec;
}

void Database::UpdatePlayerData(const GameState& playerState)
{
	Database::PlayerData& data = m_MapPlayerData[playerState.playerInfo.s_Hwid];

	data.rank	= playerState.playerInfo.rank;
	data.score	= playerState.stats.score;
}

void Database::BanPlayer(std::string& hwid, long long banTime)
{
	Database::PlayerData& playerData = m_MapPlayerData[hwid];
	playerData.banTimeInSec = std::time(nullptr) + banTime;
	playerData.banned		= true;
	playerData.permaBanned	= (banTime == -1LL);
	if (playerData.permaBanned) playerData.banTimeInSec = std::numeric_limits<int>::max();

	std::cout << "Player [" << playerData.username << "] banned till: " << Utils::TimeToString(playerData.banTimeInSec) << std::endl;
	SaveToFile();

}

void Database::AddNewPlayer(uint32_t id, shared::LoginInformation& logininfo)
{
	PlayerData data;
	strcpy_s(data.hwid, logininfo.sz_hwid);
	strcpy_s(data.username, logininfo.sz_unsername);
	data.online = true;

	m_MapPlayerData.insert({ logininfo.sz_hwid, data });
	m_MapPlayerNamesAndHwid.insert({ logininfo.sz_unsername, logininfo.sz_hwid });
}

void Database::PlayerLeft(std::string& hwid)
{
	if(IsPlayerExists(hwid) && m_MapPlayerData[hwid].rank != shared::PlayerRank::OWNER)
		m_MapPlayerData[hwid].online = false;
}

bool Database::IsPlayerNameExists(const char* name)
{
	return (m_MapPlayerNamesAndHwid.find(name) != m_MapPlayerNamesAndHwid.end());
}

bool Database::IsPlayerBanned(shared::LoginInformation& logininfo)
{
	if (!IsPlayerExists(logininfo)) return false;
	std::time_t llCurrTime = std::time(nullptr);
	Database::PlayerData& playerData = m_MapPlayerData[logininfo.sz_hwid];
	playerData.banned = (llCurrTime < playerData.banTimeInSec) || playerData.permaBanned;
	return playerData.banned;
}

bool Database::IsPlayerExists(shared::LoginInformation& logininfo)
{
	return m_MapPlayerData.count(logininfo.sz_hwid) != 0;
}

bool Database::IsPlayerExists(std::string& hwid)
{
	return m_MapPlayerData.count(hwid) != 0;
}

bool Database::IsPlayerExists(const char* sz_hwid)
{
	return m_MapPlayerData.count(sz_hwid) != 0;
}

bool Database::IsPlayerOnline(std::string& hwid)
{
	if (!IsPlayerExists(hwid)) return false;
	return m_MapPlayerData[hwid].online;
}

bool Database::IsPlayerOnline(const char* sz_hwid)
{
	if (!IsPlayerExists(sz_hwid)) return false;
	return m_MapPlayerData[sz_hwid].online;
}
shared::PlayerRank Database::GetPlayerRank(const char* sz_hwid)
{
	if (!IsPlayerExists(sz_hwid)) return shared::PlayerRank::DEFAULT;
	return  m_MapPlayerData[sz_hwid].rank;
}

#pragma once
#include "Sigleton.hpp"
#include "PacketStruct.hpp"
#include <SFML/Audio.hpp>

#include <deque>

enum class GameSounds : uint16_t
{
	PLAYER_SHOOT,
	PLAYER_HIT,
	JOIN,
	PLAYER_DIED,
	CHAT_MESSAGE,
	HIT_FEEDBACK,
	TANK_KILLED_FEEDBACK,
	PLAYER_JOIN,
	VIP_JOIN,
	PLAYER_LEFT,

	FLAG_LOST,
	FLAG_RETURN,
	FLAG_STOLE,
	FLAG_WIN,

	END
};

enum class GameMusics : uint16_t
{
	INGAME,
	MENU,
	END
};

class AudioManager : public Singleton<AudioManager>
{
public:
	AudioManager();
	~AudioManager();

	void Update();

	void PlayGameSound(GameSounds soundId, float volume);
	void PlayGameSound(GameSounds soundId, sf::Vector2f localPos, sf::Vector2f soundPos);
	void PlayGameSound(GameSounds soundId, Vector2f localPos, Vector2f soundPos, float maxVolume = 100.f);
	void PlayGameMusic(GameMusics musicId, float volume);
	void StopAllSounds();
private:
	void RemoveDeadAudio();

private:

	std::vector<sf::SoundBuffer> m_SoundBuffers;
	std::vector<std::string> m_SoundPaths;
	std::vector<std::string> m_MusicPaths;

	std::deque<sf::Sound> m_Sounds;
	std::deque<sf::Music> m_Musics;
};


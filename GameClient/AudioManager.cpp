#include "AudioManager.hpp"
#include "cl_Globals.hpp"

#include <filesystem>
#include <chrono>

#define ADD_SOUND_PATH(x) m_SoundPaths.push_back(x);
#define ADD_MUSIC_PATH(x) m_MusicPaths.push_back(x);

AudioManager::AudioManager()
{
	ADD_SOUND_PATH("sounds\\SmokyShotSound.wav");
	ADD_SOUND_PATH("sounds\\SmokyHitSound.wav");
	ADD_SOUND_PATH("sounds\\RoundStartSound1.wav");
	ADD_SOUND_PATH("sounds\\TankExplosionSound.wav");
	ADD_SOUND_PATH("sounds\\UIElementClickSound.wav");
	ADD_SOUND_PATH("sounds\\HitFeedbackSound.wav");
	ADD_SOUND_PATH("sounds\\KillTankSound.wav");
	ADD_SOUND_PATH("sounds\\joinsound.mp3");
	ADD_SOUND_PATH("sounds\\vipjoin.mp3");
	ADD_SOUND_PATH("sounds\\leavesound.mp3");

	ADD_SOUND_PATH("sounds\\FlagLost.wav");
	ADD_SOUND_PATH("sounds\\FlagReturn.wav");
	ADD_SOUND_PATH("sounds\\FlagStole.wav");
	ADD_SOUND_PATH("sounds\\FlagWin.wav");

	for (auto& path : m_SoundPaths) {
		m_SoundBuffers.emplace_back();
		m_SoundBuffers.back().loadFromFile(path);
	}

	std::string path = "sounds\\bloodpressure";
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		m_SoundBuffers.emplace_back();
		m_SoundBuffers.back().loadFromFile(entry.path().string());
	}

	ADD_MUSIC_PATH("music\\WestPrime_Ambience.wav");
}

AudioManager::~AudioManager()
{
}

void AudioManager::Update()
{
	static double nextUpdate = 0.0;

	if (nextUpdate < g_CurrentTime) {
		RemoveDeadAudio();
		nextUpdate = 1.0 + std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
}

void AudioManager::PlayGameSound(GameSounds soundId, float volume)
{
	m_Sounds.emplace_back(m_SoundBuffers.at((int)soundId));
	m_Sounds.back().setVolume(volume);
	m_Sounds.back().play();
}

void AudioManager::PlayGameSound(GameSounds soundId, sf::Vector2f localPos, sf::Vector2f soundPos)
{
	sf::Listener::setPosition(localPos.x, 0.f, localPos.y);
	m_Sounds.emplace_back(m_SoundBuffers.at((int)soundId));
	m_Sounds.back().setRelativeToListener(true);
	m_Sounds.back().setMinDistance(20.f);
	m_Sounds.back().setAttenuation(1.f);
	m_Sounds.back().setPosition(soundPos.x, 0.f, soundPos.y);
	m_Sounds.back().play();
}

void AudioManager::PlayGameSound(GameSounds soundId, Vector2f localPos, Vector2f soundPos, float maxVolume)
{
	float distance = sqrt(pow(soundPos.x - localPos.x, 2) + pow(soundPos.y - localPos.y, 2));
	float attenuationFactor = std::clamp(1.f - (distance / 1000.f), 0.f, 1.f);
	m_Sounds.emplace_back(m_SoundBuffers.at((int)soundId));
	//m_Sounds.back().setRelativeToListener(false);
	//m_Sounds.back().setMinDistance(10.f);
	//m_Sounds.back().setAttenuation(10.f);
	m_Sounds.back().setVolume(maxVolume * attenuationFactor);
	//m_Sounds.back().setPosition(soundPos.x, 0.f, soundPos.y);
	m_Sounds.back().play();
}

void AudioManager::PlayGameMusic(GameMusics musicId, float volume)
{
	m_Musics.emplace_back();
	m_Musics.back().openFromFile(m_MusicPaths.at((int)musicId));
	m_Musics.back().setVolume(volume);
	m_Musics.back().play();
}

void AudioManager::StopAllSounds()
{
	for (auto& sound : m_Sounds)
		sound.stop();
	m_Sounds.clear();
}

void AudioManager::RemoveDeadAudio()
{
	while (!m_Sounds.empty() && m_Sounds.front().getStatus() == sf::Sound::Stopped)
		m_Sounds.pop_front();

	while (!m_Musics.empty() && m_Musics.front().getStatus() == sf::Music::Stopped)
		m_Musics.pop_front();
}

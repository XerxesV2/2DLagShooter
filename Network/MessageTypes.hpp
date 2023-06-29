#pragma once
#include "includes.hpp"

enum class MessageTypes : uint8_t
{
	ServerAccept,
	ServerRefuse,
	ServerPing,
	MessageAll,
	ServerMessage,
	BadClient
};

enum class GameMessages : uint8_t
{
	GetStatus,

	ServerAccept,
	ServerRefuse,
	ServerPing,
	MessageAll,
	ServerMessage,
	BadClient,

	Login,
	LoginAccept,
	LoginRefuse,
	RegisterClient,
	SetClientID,

	AddPlayer,
	RemovePlayer,
	UpdatePlayerMovement,
	UpdatePlayerActions,
	GameStateUpdate,
	PlayerDied,

	UdpTest,
	AssignUdpPort,

	ChatMessage,
	ChatCommand,

	UpdateStats,

	None
};

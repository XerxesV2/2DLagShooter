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
	BannedClient,
	Disconnect,

	Login,
	LoginAccept,
	LoginRefuse,
	AutoLogin,
	AutoLoginFail,
	AutoLoginSuccess,
	RegisterClient,
	SetClientID,
	LoginNameTaken,

	AddPlayer,
	RemovePlayer,
	UpdatePlayerMovement,
	UpdatePlayerActions,
	WorldState,
	GameStateUpdate,
	FlagStateUpdate,
	PlayerDied,

	UdpTest,
	AssignUdpPort,

	ChatMessage,
	ChatCommand,

	UpdateStatus,
	NoPermission,
	MysteriousError,

	None
};

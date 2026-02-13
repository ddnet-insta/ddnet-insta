#ifndef INSTA_SERVER_DEAD_SPEC_CONTROLLER_H
#define INSTA_SERVER_DEAD_SPEC_CONTROLLER_H

#include <engine/shared/protocol.h>

class CPlayer;
class IGameController;
class CGameContext;
class IServer;

class CDeadSpecPlayer
{
public:
	bool m_WantsToJoinSpectators = false;

	// for last man standing modes where dead players get moved to spectators
	// to differentitate between players that want to stay spec and the ones
	// that wait for the next round to play
	bool m_WantsToStaySpectator = false;
};

class CDeadSpecController
{
	IGameController *m_pController = nullptr;
	CGameContext *m_pGameServer = nullptr;
	CGameContext *GameServer();
	const CGameContext *GameServer() const;
	IGameController *Controller();
	const IGameController *Controller() const;
	IServer *Server();
	const IServer *Server() const;

	CDeadSpecPlayer *m_apPlayers[MAX_CLIENTS] = {nullptr};

	public:

	CDeadSpecController(IGameController *pController, CGameContext *pGameServer);
	~CDeadSpecController();

	void OnPlayerConnect(CPlayer *pPlayer);
	void OnPlayerDisconnect(CPlayer *pPlayer);

	/*
		Function: OnSetTeamNetMessage
			hooks into CGameContext::OnSetTeamNetMessage()
			before any spam protection check

			See also CanJoinTeam() which is called after the validation

		Returns:
			return true to not run the rest of CGameContext::OnSetTeamNetMessage()
	*/
	bool OnSetTeamNetMessage(const class CNetMsg_Cl_SetTeam *pMsg, int ClientId);

	// Kills the player and moves them to the spectators
	// they can not join the game until `RespawnPlayer()`
	// is called
	void KillPlayer(CPlayer *pPlayer, int KillerId);

	// Moves dead spectators back into the game
	void RespawnPlayer(CPlayer *pPlayer);

	// Move all players back to the game on round end for example
	// but keep permanent spectators spectators
	void RespawnAllPlayers();
};

#endif

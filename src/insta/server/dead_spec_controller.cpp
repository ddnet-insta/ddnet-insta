#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <insta/server/dead_spec_controller.h>

CGameContext *CDeadSpecController::GameServer()
{
	return m_pGameServer;
}

const CGameContext *CDeadSpecController::GameServer() const
{
	return m_pGameServer;
}

IGameController *CDeadSpecController::Controller()
{
	return m_pController;
}

const IGameController *CDeadSpecController::Controller() const
{
	return m_pController;
}

IServer *CDeadSpecController::Server()
{
	return GameServer()->Server();
}

const IServer *CDeadSpecController::Server() const
{
	return GameServer()->Server();
}

CDeadSpecController::CDeadSpecController(IGameController *pController, CGameContext *pGameServer)
{
	m_pController = pController;
	m_pGameServer = pGameServer;
}

CDeadSpecController::~CDeadSpecController()
{
	for(CDeadSpecPlayer *pPlayer : m_apPlayers)
	{
		delete pPlayer;
		pPlayer = nullptr;
	}
}

void CDeadSpecController::OnPlayerConnect(CPlayer *pPlayer)
{
	dbg_assert(m_apPlayers[pPlayer->GetCid()] == nullptr, "dead spec slot for cid=%d is reused", pPlayer->GetCid());
	m_apPlayers[pPlayer->GetCid()] = new CDeadSpecPlayer();
}

void CDeadSpecController::OnPlayerDisconnect(CPlayer *pPlayer)
{
	dbg_assert(m_apPlayers[pPlayer->GetCid()], "dead spec slot for cid=%d is not set", pPlayer->GetCid());
	delete m_apPlayers[pPlayer->GetCid()];
	m_apPlayers[pPlayer->GetCid()] = nullptr;
}

bool CDeadSpecController::OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId)
{
	if(GameServer()->m_World.m_Paused)
		return false;
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;

	CDeadSpecPlayer *pDeadSpec = m_apPlayers[ClientId];
	int Team = pMsg->m_Team;
	if(Server()->IsSixup(ClientId) && g_Config.m_SvSpectatorVotes && g_Config.m_SvSpectatorVotesSixup && pPlayer->m_IsFakeDeadSpec)
	{
		if(Team == TEAM_SPECTATORS)
		{
			// when a sixup fake spec tries to join spectators
			// he actually tries to join team red
			Team = TEAM_RED;
		}
	}

	if(
		(Server()->IsSixup(ClientId) && pPlayer->m_IsDead && Team == TEAM_SPECTATORS) ||
		(!Server()->IsSixup(ClientId) && pPlayer->m_IsDead && Team == TEAM_RED))
	{
		pDeadSpec->m_WantsToJoinSpectators = !pDeadSpec->m_WantsToJoinSpectators;
		char aBuf[512] = "";
		if(pDeadSpec->m_WantsToJoinSpectators)
			Controller()->YouWillJoinSpecMessage(pPlayer, aBuf, sizeof(aBuf));
		else
			Controller()->YouWillJoinGameMessage(pPlayer, aBuf, sizeof(aBuf));

		GameServer()->SendBroadcast(aBuf, ClientId);
		return true;
	}

	return false;
}

void CDeadSpecController::KillPlayer(CPlayer *pPlayer, int KillerId)
{
	pPlayer->m_IsDead = true;
	pPlayer->m_KillerId = KillerId;

	// force death screen open as long as sv_enemy_kill_respawn_delay_ms specifies
	// then move to spectators after that. Instead of moving to spectators instantly.
	// helps with ddnet client losing spectator focus because
	// of clicking directly after death
	// https://github.com/ddnet-insta/ddnet-insta/issues/447
	pPlayer->m_ForceTeam = {
		.m_Tick = pPlayer->m_RespawnTick,
		.m_Team = TEAM_SPECTATORS,
		.m_SpectatorId = KillerId};
}

void CDeadSpecController::RespawnPlayer(CPlayer *pPlayer)
{
	pPlayer->m_IsDead = false;
	pPlayer->m_KillerId = -1;
	CDeadSpecPlayer *pDeadSpec = m_apPlayers[pPlayer->GetCid()];

	if(pDeadSpec->m_WantsToJoinSpectators)
	{
		Controller()->DoTeamChange(pPlayer, TEAM_SPECTATORS, true);
		pDeadSpec->m_WantsToJoinSpectators = false;
	}
	else
	{
		// TODO: TEAM_RED does not work for team modes here
		//       there is no team survival mode yet but there will be

		// release player back into the world
		// if the kill is old
		pPlayer->SetTeamNoKill(TEAM_RED);

		// abort move to team spectators
		// if the kill is recent
		pPlayer->m_ForceTeam.m_Tick = 0;
	}
}

void CDeadSpecController::RespawnAllPlayers()
{
	log_info("deadspec", "respawning all...");

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		// abort move to team spectators
		// if the kill is recent
		pPlayer->m_ForceTeam.m_Tick = 0;

		pPlayer->m_KillerId = -1;
		pPlayer->m_IsDead = false;

		CDeadSpecPlayer *pDeadSpec = m_apPlayers[pPlayer->GetCid()];

		if(pDeadSpec->m_WantsToJoinSpectators)
		{
			Controller()->DoTeamChange(pPlayer, TEAM_SPECTATORS, true);
			pDeadSpec->m_WantsToJoinSpectators = false;
			pDeadSpec->m_WantsToStaySpectator = true;
			log_info("deadspec", "  cid=%d wants to join spec", pPlayer->GetCid());
			continue;
		}
		// do not auto join players that are
		// intentionally spectator on round start
		if(pDeadSpec->m_WantsToStaySpectator)
		{
			log_info("deadspec", "  cid=%d wants to stay spec", pPlayer->GetCid());
			continue;
		}

		log_info("deadspec", "  cid=%d moved to game ", pPlayer->GetCid());

		// TODO: support multiple teams
		pPlayer->SetTeam(TEAM_GAME, false);
		pPlayer->m_RespawnTick = 0;
		pPlayer->TryRespawn();
	}
}

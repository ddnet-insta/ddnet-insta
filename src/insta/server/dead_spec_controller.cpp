#include <base/dbg.h>

#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
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
	if(Controller()->IsGamePaused())
		return false;
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	CDeadSpecPlayer *pDeadSpec = m_apPlayers[ClientId];
	if(!pDeadSpec)
	{
		// log_error("deadspec", "cid=%d tried to switch teams but has no dead spec info", pPlayer->GetCid());
		return false;
	}

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

		// log_info(
		// 	"deadspec",
		// 	"cid=%d attempted to switch teams and will join the %s as soon as possible",
		// 	pPlayer->GetCid(),
		// 	pDeadSpec->m_WantsToJoinSpectators ? "spectators" : "game");

		GameServer()->SendBroadcast(aBuf, ClientId);
		return true;
	}

	if(Team == TEAM_SPECTATORS && !pPlayer->m_IsDead)
	{
		// it is a bit fragile to copy paste ddnets ratelimits
		// but it is not too bad if we dsync
		//
		// this is just a small user experience improvement
		// if someone joins for example spectators too fast after joining the game
		// ddnet will drop it because of ratelimit and nothing happens
		//
		// in that case we do not want in for example zCatch that that user then gets
		// moved to spectators after playing a bit and getting caught and getting released again
		//
		// another option is to use a timer here and expire join spectator requests but i feel like
		// that would be even worse to maintain
		if(!DDNetWillBlockOnSetTeamNetmessage(pPlayer, Team))
		{
			pDeadSpec->m_WantsToJoinSpectators = true;
			// log_info(
			// 	"deadspec",
			// 	"alive player cid=%d name='%s' intentionally tried to join spectators and will stay there as soon as it happened",
			// 	pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));
		}
	}

	return false;
}

bool CDeadSpecController::DDNetWillBlockOnSetTeamNetmessage(const CPlayer *pPlayer, int Team) const
{
	// has to be kept in sync with the checks in here
	// void CGameContext::OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId)

	if(g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam + Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay > Server()->Tick())
		return true;

	// Kill Protection
	const CCharacter *pChr = pPlayer->GetCharacter();
	if(pChr)
	{
		int CurrTime = (Server()->Tick() - pChr->m_StartTime) / Server()->TickSpeed();
		if(g_Config.m_SvKillProtection != 0 && CurrTime >= (60 * g_Config.m_SvKillProtection) && pChr->m_DDRaceState == ERaceState::STARTED)
			return true;
	}
	if(pPlayer->m_TeamChangeTick > Server()->Tick())
		return true;
	return false;
}

void CDeadSpecController::DoTeamChange(const CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	CDeadSpecPlayer *pDeadSpec = m_apPlayers[pPlayer->GetCid()];
	if(!pDeadSpec)
		return;

	if(Team == TEAM_SPECTATORS)
	{
		if(pDeadSpec->m_WantsToJoinSpectators)
		{
			pDeadSpec->m_WantsToJoinSpectators = false;
			pDeadSpec->m_WantsToStaySpectator = true;

			// log_info(
			// 	"deadspec",
			// 	"cid=%d name='%s' successfully joined spectators and will stay there",
			// 	pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));
		}
	}

	if(Team != TEAM_SPECTATORS)
	{
		if(pDeadSpec->m_WantsToStaySpectator)
		{
			// log_info(
			// 	"deadspec",
			// 	"cid=%d name='%s' no longer wants to stay spectator because they joined the game",
			// 	pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));
			pDeadSpec->m_WantsToStaySpectator = false;
		}
	}
}

void CDeadSpecController::KillPlayer(CPlayer *pPlayer, int KillerId)
{
	pPlayer->m_IsDead = true;
	pPlayer->m_KillerId = KillerId;
	pPlayer->m_TeamBeforeDeath = pPlayer->GetTeam();

	// we also support marking spectators as dead
	// this is useful for zCatch where players get caught
	// by the leading player on join
	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		// force death screen open as long as sv_enemy_kill_respawn_delay_ms specifies
		// then move to spectators after that. Instead of moving to spectators instantly.
		// helps with ddnet client losing spectator focus because
		// of clicking directly after death
		// https://github.com/ddnet-insta/ddnet-insta/issues/447
		pPlayer->m_ForceTeam = {
			.m_Tick = std::max(pPlayer->m_RespawnTick - 1, Server()->Tick()),
			.m_Team = TEAM_SPECTATORS,
			.m_SpectatorId = KillerId};
	}
	else
	{
		// https://github.com/ddnet-insta/ddnet-insta/issues/622
		// if a player joins as spec in a running zcatch round with leader
		// they will not be moved to spectators they will stay there
		// in that case they have to be explicitly set to follow the leading player
		pPlayer->SetSpectatorId(KillerId);
	}
}

void CDeadSpecController::RespawnPlayer(CPlayer *pPlayer)
{
	// abort move to team spectators
	// if the kill is recent
	pPlayer->m_ForceTeam.m_Tick = 0;

	pPlayer->m_KillerId = -1;
	pPlayer->m_IsDead = false;

	CDeadSpecPlayer *pDeadSpec = m_apPlayers[pPlayer->GetCid()];
	if(!pDeadSpec)
	{
		// log_warn("deadspec", " cid=%d is missing a dead spec instance", pPlayer->GetCid());
		return;
	}

	if(pDeadSpec->m_WantsToJoinSpectators)
	{
		// log_info("deadspec", "  cid=%d wants to join spec (current team %d)", pPlayer->GetCid(), pPlayer->GetTeam());
		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
		{
			// if dead players want to join specs and get released
			// they move from dead fake spec to real alive spec
			// in that case DoTeamChange() returns early and prints no chat message
			// so we have to print it here
			// https://github.com/ddnet-insta/ddnet-insta/issues/613
			char aBuf[512];
			str_format(
				aBuf,
				sizeof(aBuf),
				"'%s' joined the %s",
				Server()->ClientName(pPlayer->GetCid()),
				GameServer()->m_pController->GetTeamName(TEAM_SPECTATORS));
			GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);

			protocol7::CNetMsg_Sv_Team Msg;
			Msg.m_ClientId = pPlayer->GetCid();
			Msg.m_Team = TEAM_SPECTATORS;
			Msg.m_Silent = false;
			Msg.m_CooldownTick = pPlayer->m_LastSetTeam + Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
		}

		Controller()->DoTeamChange(pPlayer, TEAM_SPECTATORS, true);
		pDeadSpec->m_WantsToJoinSpectators = false;
		pDeadSpec->m_WantsToStaySpectator = true;
		dbg_assert(pPlayer->GetTeam() == TEAM_SPECTATORS, "  cid=%d failed to join spectators", pPlayer->GetCid());
		return;
	}
	// do not auto join players that are
	// intentionally spectator on round start
	if(pDeadSpec->m_WantsToStaySpectator)
	{
		// log_info("deadspec", "  cid=%d wants to stay spec", pPlayer->GetCid());
		return;
	}

	// log_info("deadspec", "  cid=%d moved to game (current team: %d)", pPlayer->GetCid(), pPlayer->GetTeam());

	// do not kill the winner in the round end screen
	// https://github.com/ddnet-insta/ddnet-insta/issues/604
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		// log_info("deadspec", "  cid=%d name='%s' moved to game actually", pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));

		GameServer()->m_pController->DoTeamChange(pPlayer, pPlayer->m_TeamBeforeDeath, false);
	}
}

void CDeadSpecController::RespawnAllPlayers()
{
	// log_info("deadspec", "respawning all...");

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		RespawnPlayer(pPlayer);
	}
}

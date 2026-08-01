#include "ltsblock.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/dead_spec_controller.h>
#include <insta/server/gamemodes/ddrace/block/block.h>

CGameControllerLTSBlock::CGameControllerLTSBlock(class CGameContext *pGameServer) :
	CGameControllerBlock(pGameServer)
{
	m_pGameType = "ltsblock";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_DefaultWeapon = WEAPON_HAMMER;

	m_pDeadSpecController = new CDeadSpecController(this, pGameServer);

	m_pStatsTable = "ltsblock";
	m_pExtraColumns = nullptr;
	Db()->Stats()->SetExtraColumns(m_pExtraColumns);
	Db()->Stats()->CreateTable(m_pStatsTable);
}

void CGameControllerLTSBlock::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ltsblock created by ByFox. Idea by Brokecdx-. For tpl.world",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

int CGameControllerLTSBlock::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponId)
{
	CGameControllerBlock::OnCharacterDeath(pVictim, pKiller, WeaponId);

	// skip bookkeeping for the artificial kills during a round reset so survivors don't get marked as dead
	if(m_RoundReset)
		return 0;

	// only apply dead-spec logic when a real round is in progress (both teams had players)
	if(m_RoundActive)
	{
		m_pDeadSpecController->KillPlayer(pVictim->GetPlayer(), pKiller ? pKiller->GetCid() : -1);
	}

	// track kill/death stats like regular block, but don't add team score
	return 0;
}

bool CGameControllerLTSBlock::DoWincheckRound()
{
	if(IGameController::DoWincheckRound())
		return true;

	int AliveRed = 0;
	int AliveBlue = 0;
	CountAlivePlayersByTeam(AliveRed, AliveBlue);

	if(AliveRed > 0 && AliveBlue > 0)
		return false;

	// dead players only exist during an active round (OnCharacterDeath only
	// calls KillPlayer when m_RoundActive) and an active round implies both
	// teams had players, so no dead player bookkeeping is needed here
	if(!m_RoundActive)
		return false;

	// both teams wiped on the same tick, restart without awarding a point
	if(AliveRed == 0 && AliveBlue == 0)
	{
		GameServer()->SendChat(-1, TEAM_ALL, "Draw! Both teams were eliminated. Starting new round.");
		StartNewRound();
		return false;
	}

	int WinningTeam = (AliveRed > 0) ? TEAM_RED : TEAM_BLUE;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s team wins the round!", GetTeamName(WinningTeam));
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);

	AddTeamscore(WinningTeam, 1);

	// check if the match is over after adding the point
	if(IGameController::DoWincheckRound())
		return true;

	StartNewRound();
	return false;
}

void CGameControllerLTSBlock::Tick()
{
	CGameControllerBlock::Tick();

	int AliveRed = 0;
	int AliveBlue = 0;
	CountAlivePlayersByTeam(AliveRed, AliveBlue);

	if(m_RoundActive && AliveRed + AliveBlue == 0)
	{
		m_RoundActive = false;
		m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;
		m_pDeadSpecController->RespawnAllPlayers();
		AliveRed = 0;
		AliveBlue = 0;
		CountAlivePlayersByTeam(AliveRed, AliveBlue);
	}

	// activate the round once both teams have at least one player
	// mirrors bombs pattern: OnRoundStart() fires at server init with no players
	// so we must not rely on it to set m_RoundActive = true
	if(!m_RoundActive && !m_Warmup)
	{
		if(AliveRed > 0 && AliveBlue > 0)
		{
			m_RoundActive = true;
			GameServer()->SendBroadcast("", -1);
		}
		else
		{
			switch(Server()->Tick() % (Server()->TickSpeed() * 3))
			{
			case 50:
				GameServer()->SendBroadcast("Waiting for players.", -1);
				break;
			case 100:
				GameServer()->SendBroadcast("Waiting for players..", -1);
				break;
			case 0:
				GameServer()->SendBroadcast("Waiting for players...", -1);
				break;
			}
		}
	}

	HandleFrozenTeamTimeout(AliveRed, AliveBlue);
}

void CGameControllerLTSBlock::StartNewRound()
{
	m_RoundActive = false;
	m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;
	m_RoundStartTick = Server()->Tick();

	// bring killed players back to their teams
	m_pDeadSpecController->RespawnAllPlayers();

	// kill all characters so everyone respawns fresh; m_RoundReset stops OnCharacterDeath from marking survivors as dead
	m_RoundReset = true;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		pPlayer->m_LastToucher.reset();
		pPlayer->KillCharacter(WEAPON_GAME, false);
	}
	m_RoundReset = false;

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;

		pPlayer->m_RespawnTick = Server()->Tick();
		pPlayer->Respawn();
	}
}

void CGameControllerLTSBlock::OnRoundStart()
{
	CGameControllerBlock::OnRoundStart();

	// bring killed players back to their teams
	m_pDeadSpecController->RespawnAllPlayers();

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		if(pPlayer->GetCharacter())
			continue;

		pPlayer->m_RespawnTick = Server()->Tick();
		pPlayer->Respawn();
	}
	// m_RoundActive is not set here, Tick() will set it once both teams have a player
}

void CGameControllerLTSBlock::OnRoundEnd()
{
	CGameControllerBlock::OnRoundEnd();
	m_RoundActive = false;
	m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;
}

void CGameControllerLTSBlock::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerBlock::OnPlayerConnect(pPlayer);

	// Prevent bypassing death by reconnecting mid-round
	// m_RoundActive is only true once Tick() has confirmed both teams have players,
	// so a genuine first/second joiner will never be blocked here
	if(m_RoundActive)
	{
		// KillPlayer remembers the team they were assigned on connect so
		// RespawnAllPlayers can move them back into red/blue instead of
		// leaving them in spectators forever
		m_pDeadSpecController->KillPlayer(pPlayer, -1);
		GameServer()->SendChatTarget(pPlayer->GetCid(), "You have to wait for the round to end before you can join");
	}
}

void CGameControllerLTSBlock::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	CGameControllerBlock::OnPlayerDisconnect(pPlayer, pReason);
	if(m_RoundActive)
	{
		int AliveRed = 0, AliveBlue = 0;
		CountAlivePlayersByTeam(AliveRed, AliveBlue);
		if(AliveRed + AliveBlue == 0)
		{
			m_RoundActive = false;
			m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;
			m_pDeadSpecController->RespawnAllPlayers();
		}
	}
}

void CGameControllerLTSBlock::YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the spectators once the match ends", MsgLen);
}

void CGameControllerLTSBlock::YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the game once the match ends", MsgLen);
}

void CGameControllerLTSBlock::CountAlivePlayersByTeam(int &AliveRed, int &AliveBlue) const
{
	AliveRed = 0;
	AliveBlue = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->m_IsDead)
			continue;

		if(pPlayer->GetTeam() == TEAM_RED)
			AliveRed++;
		else if(pPlayer->GetTeam() == TEAM_BLUE)
			AliveBlue++;
	}
}

bool CGameControllerLTSBlock::HandleFrozenTeamTimeout(int AliveRed, int AliveBlue)
{
	if(!m_RoundActive || m_Warmup > 0)
	{
		m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;
		return false;
	}

	bool AllRedFrozen = AliveRed > 0;
	bool AllBlueFrozen = AliveBlue > 0;

	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->m_IsDead)
			continue;

		int Team = pPlayer->GetTeam();
		if(Team != TEAM_RED && Team != TEAM_BLUE)
			continue;

		const CCharacter *pChr = pPlayer->GetCharacter();
		if(pChr && !pChr->Core()->m_IsInFreeze)
			(Team == TEAM_RED ? AllRedFrozen : AllBlueFrozen) = false;
	}

	if(AllRedFrozen && AllBlueFrozen && m_RoundStartTick + Server()->TickSpeed() * 3 < Server()->Tick())
	{
		// both teams frozen at the same time is an instant draw
		GameServer()->SendChat(-1, TEAM_ALL, "Draw! Both teams were fully frozen. Starting new round.");
		m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;
		StartNewRound();
		return true;
	}

	m_RedTeamFrozenTicks = (AllRedFrozen && AliveBlue > 0) ? m_RedTeamFrozenTicks + 1 : 0;
	m_BlueTeamFrozenTicks = (AllBlueFrozen && AliveRed > 0) ? m_BlueTeamFrozenTicks + 1 : 0;

	const int FreezeLossTicks = 8 * Server()->TickSpeed();
	const bool RedTimedOut = m_RedTeamFrozenTicks > FreezeLossTicks;
	const bool BlueTimedOut = m_BlueTeamFrozenTicks > FreezeLossTicks;

	if(!RedTimedOut && !BlueTimedOut)
		return false;

	m_RedTeamFrozenTicks = m_BlueTeamFrozenTicks = 0;

	int LosingTeam = RedTimedOut ? TEAM_RED : TEAM_BLUE;
	int WinningTeam = (LosingTeam == TEAM_RED) ? TEAM_BLUE : TEAM_RED;

	// forcekill the frozen team, but skip death bookkeeping
	m_RoundReset = true;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() != LosingTeam)
			continue;
		if(CCharacter *pChr = pPlayer->GetCharacter())
		{
			pPlayer->m_LastToucher.reset();
			pChr->Die(-1, WEAPON_GAME, false);
			pPlayer->m_Stats.m_Deaths++;
		}
	}
	m_RoundReset = false;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s team was fully frozen for 8 seconds and loses the round!", GetTeamName(LosingTeam));
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);

	AddTeamscore(WinningTeam, 1);
	if(IGameController::DoWincheckRound())
		return true;

	StartNewRound();
	return true;
}

REGISTER_GAMEMODE(ltsblock, CGameControllerLTSBlock(pGameServer));

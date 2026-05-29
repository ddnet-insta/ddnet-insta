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
	m_DefaultWeapon = WEAPON_GUN;
	m_IsVanillaGameType = false;

	m_pDeadSpecController = new CDeadSpecController(this, pGameServer);

	m_pStatsTable = "ltsblock";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);

	for(int &Team : m_aPreDeathTeam)
		Team = TEAM_SPECTATORS;
}

CGameControllerLTSBlock::~CGameControllerLTSBlock() = default;

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

void CGameControllerLTSBlock::RestorePlayersFromPreDeathTeam(bool OnlyDeadPlayers)
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(OnlyDeadPlayers && !pPlayer->m_IsDead)
			continue;

		int OrigTeam = m_aPreDeathTeam[pPlayer->GetCid()];
		if(OrigTeam != TEAM_RED && OrigTeam != TEAM_BLUE)
			continue;

		pPlayer->m_ForceTeam.m_Tick = 0;
		pPlayer->m_IsDead = false;
		pPlayer->m_KillerId = -1;
		m_aPreDeathTeam[pPlayer->GetCid()] = TEAM_SPECTATORS;

		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			DoTeamChange(pPlayer, OrigTeam, false);
	}
}

void CGameControllerLTSBlock::RespawnNonSpectatorPlayers(bool OnlyWithoutCharacter)
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		if(OnlyWithoutCharacter && pPlayer->GetCharacter())
			continue;

		// zero out any respawn cooldown so everyone spawns on the same tick
		pPlayer->m_RespawnTick = Server()->Tick();
		pPlayer->Respawn();
	}
}

bool CGameControllerLTSBlock::IsCharacterFrozen(const CCharacter *pChr) const
{
	if(!pChr)
		return false;

	const CCharacterCore *pCore = pChr->Core();
	if(!pCore)
		return false;

	return pCore->m_IsInFreeze;
}

void CGameControllerLTSBlock::ResetFrozenTeamTimers()
{
	m_RedTeamFrozenTicks = 0;
	m_BlueTeamFrozenTicks = 0;
}

bool CGameControllerLTSBlock::HandleFrozenTeamTimeout(int AliveRed, int AliveBlue)
{
	if(!m_bRoundActive || m_Warmup > 0)
	{
		ResetFrozenTeamTimers();
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
		if(Team == TEAM_RED)
		{
			if(!IsCharacterFrozen(pChr))
				AllRedFrozen = false;
		}
		else
		{
			if(!IsCharacterFrozen(pChr))
				AllBlueFrozen = false;
		}
	}

	m_RedTeamFrozenTicks = (AllRedFrozen && AliveBlue > 0) ? m_RedTeamFrozenTicks + 1 : 0;
	m_BlueTeamFrozenTicks = (AllBlueFrozen && AliveRed > 0) ? m_BlueTeamFrozenTicks + 1 : 0;

	const int FreezeLossTicks = 5 * Server()->TickSpeed(); // hardcoded to 5 seconds, don't know if it's worth making this configurable
	const bool RedTimedOut = m_RedTeamFrozenTicks > FreezeLossTicks;
	const bool BlueTimedOut = m_BlueTeamFrozenTicks > FreezeLossTicks;
	if(!RedTimedOut && !BlueTimedOut)
		return false;

	ResetFrozenTeamTimers();

	if(RedTimedOut && BlueTimedOut)
	{
		GameServer()->SendChat(-1, TEAM_ALL, "Both teams stayed frozen too long. Starting new round.");
		StartNewRound();
		return true;
	}

	int LosingTeam = RedTimedOut ? TEAM_RED : TEAM_BLUE;
	int WinningTeam = (LosingTeam == TEAM_RED) ? TEAM_BLUE : TEAM_RED;

	// forcekill the frozen team, but skip death bookkeeping
	m_bRoundReset = true;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() != LosingTeam)
			continue;
		if(CCharacter *pChr = pPlayer->GetCharacter())
			pChr->Die(-1, WEAPON_GAME, false);
	}
	m_bRoundReset = false;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s team was fully frozen for 5 seconds and loses the round!", GetTeamName(LosingTeam));
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);

	AddTeamscore(WinningTeam, 1);
	if(IGameController::DoWincheckRound())
		return true;

	StartNewRound();
	return true;
}

void CGameControllerLTSBlock::ResetRoundStateIfEmpty()
{
	if(!m_bRoundActive)
		return;
	int AliveRed = 0;
	int AliveBlue = 0;
	CountAlivePlayersByTeam(AliveRed, AliveBlue);

	// if no fighters are left, the round cannot progress and all dead-spec players
	// must be released so they can join the next fight normally
	if(AliveRed + AliveBlue > 0)
		return;

	m_bRoundActive = false;
	ResetFrozenTeamTimers();
	m_pDeadSpecController->RespawnAllPlayers();

	for(int &Team : m_aPreDeathTeam)
		Team = TEAM_SPECTATORS;
}

int CGameControllerLTSBlock::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponId)
{
	// skip bookkeeping for the artificial kills during a round reset so survivors don't get marked as dead
	if(m_bRoundReset)
		return 0;

	// only apply dead-spec logic when a real round is in progress (both teams had players)
	if(m_bRoundActive)
	{
		// remember the team so we can respawn them at the start of the next round
		m_aPreDeathTeam[pVictim->GetPlayer()->GetCid()] = pVictim->GetPlayer()->GetTeam();
		m_pDeadSpecController->KillPlayer(pVictim->GetPlayer(), pKiller ? pKiller->GetCid() : -1);
	}

	// track kill/death stats like regular block, but don't add team score
	return CGameControllerBlock::OnCharacterDeath(pVictim, pKiller, WeaponId);
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

	// count dead players per team (they're in spec with m_IsDead set)
	int DeadRed = 0, DeadBlue = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || !pPlayer->m_IsDead)
			continue;
		int Cid = pPlayer->GetCid();
		if(m_aPreDeathTeam[Cid] == TEAM_RED)
			DeadRed++;
		else if(m_aPreDeathTeam[Cid] == TEAM_BLUE)
			DeadBlue++;
	}

	// don't start a new round if one team never had any players
	if(AliveRed + DeadRed == 0 || AliveBlue + DeadBlue == 0)
		return false;

	// both teams wiped on the same tick, restart without awarding a point
	if(AliveRed == 0 && AliveBlue == 0)
	{
		GameServer()->SendChat(-1, TEAM_ALL, "Both teams were eliminated! Starting new round.");
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

void CGameControllerLTSBlock::StartNewRound()
{
	m_bRoundActive = false;
	ResetFrozenTeamTimers();

	// bring killed players back to their teams
	RestorePlayersFromPreDeathTeam(true);

	// kill all characters so everyone respawns fresh; m_bRoundReset stops OnCharacterDeath from marking survivors as dead
	m_bRoundReset = true;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		if(pPlayer->GetCharacter())
			pPlayer->KillCharacter(WEAPON_GAME, false);
	}
	m_bRoundReset = false;

	RespawnNonSpectatorPlayers(false);
}

void CGameControllerLTSBlock::Tick()
{
	CGameControllerBlock::Tick();
	ResetRoundStateIfEmpty();

	int AliveRed = 0;
	int AliveBlue = 0;
	CountAlivePlayersByTeam(AliveRed, AliveBlue);

	// activate the round once both teams have at least one player
	// mirrors bombs pattern: OnRoundStart() fires at server init with no players
	// so we must not rely on it to set m_bRoundActive = true
	if(!m_bRoundActive && !m_Warmup && AliveRed > 0 && AliveBlue > 0)
	{
		m_bRoundActive = true;
	}

	HandleFrozenTeamTimeout(AliveRed, AliveBlue);
}

void CGameControllerLTSBlock::OnRoundStart()
{
	CGameControllerBlock::OnRoundStart();

	// restore players who were dead-speced at the end of the previous match
	// m_aPreDeathTeam is TEAM_SPECTATORS for players who never competed
	RestorePlayersFromPreDeathTeam(false);
	RespawnNonSpectatorPlayers(true);
	// m_bRoundActive is not set here, Tick() will set it once both teams have a player
}

void CGameControllerLTSBlock::OnRoundEnd()
{
	CGameControllerBlock::OnRoundEnd();
	m_bRoundActive = false;
	ResetFrozenTeamTimers();
}

void CGameControllerLTSBlock::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerBlock::OnPlayerConnect(pPlayer);
	// reset slot so a reconnecting player doesn't inherit the previous occupant's team
	m_aPreDeathTeam[pPlayer->GetCid()] = TEAM_SPECTATORS;

	// Prevent bypassing death by reconnecting mid-round
	// m_bRoundActive is only true once Tick() has confirmed both teams have players,
	// so a genuine first/second joiner will never be blocked here
	if(m_bRoundActive)
	{
		// Keep the team they were assigned on connect so StartNewRound can move
		// them back into red/blue instead of leaving them in spectators forever
		m_aPreDeathTeam[pPlayer->GetCid()] = pPlayer->GetTeam();
		m_pDeadSpecController->KillPlayer(pPlayer, -1);
		GameServer()->SendChatTarget(pPlayer->GetCid(), "You have to wait for the round to end before you can join");
	}
}

void CGameControllerLTSBlock::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	CGameControllerBlock::OnPlayerDisconnect(pPlayer, pReason);
	ResetRoundStateIfEmpty();
}

void CGameControllerLTSBlock::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ltsblock created by Noa for tpl.world",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerLTSBlock::YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the spectators once the match ends", MsgLen);
}

void CGameControllerLTSBlock::YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the game once the match ends", MsgLen);
}

REGISTER_GAMEMODE(ltsblock, CGameControllerLTSBlock(pGameServer));

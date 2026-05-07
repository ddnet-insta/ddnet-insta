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

int CGameControllerLTSBlock::CountAlivePlayersTeam(int Team) const
{
	int Count = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(pPlayer && !pPlayer->m_IsDead && pPlayer->GetTeam() == Team)
			Count++;
	}
	return Count;
}

int CGameControllerLTSBlock::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponId)
{
	// skip bookkeeping for the artificial kills during a round reset so survivors don't get marked as dead
	if(m_bRoundReset)
		return 0;

	// remember the team so we can respawn them at the start of the next round
	m_aPreDeathTeam[pVictim->GetPlayer()->GetCid()] = pVictim->GetPlayer()->GetTeam();

	m_pDeadSpecController->KillPlayer(pVictim->GetPlayer(), pKiller ? pKiller->GetCid() : -1);

	// track kill/death stats like regular block, but don't add team score
	return CGameControllerBlock::OnCharacterDeath(pVictim, pKiller, WeaponId);
}

bool CGameControllerLTSBlock::DoWincheckRound()
{
	if(IGameController::DoWincheckRound())
		return true;

	int AliveRed = CountAlivePlayersTeam(TEAM_RED);
	int AliveBlue = CountAlivePlayersTeam(TEAM_BLUE);

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
	// bring killed players back to their teams
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || !pPlayer->m_IsDead)
			continue;

		int OrigTeam = m_aPreDeathTeam[pPlayer->GetCid()];

		// skip permanent spectators just in case
		if(OrigTeam != TEAM_RED && OrigTeam != TEAM_BLUE)
			continue;

		pPlayer->m_ForceTeam.m_Tick = 0;
		pPlayer->m_IsDead = false;
		pPlayer->m_KillerId = -1;
		m_aPreDeathTeam[pPlayer->GetCid()] = TEAM_SPECTATORS;

		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			DoTeamChange(pPlayer, OrigTeam, false);
	}

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

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		// zero out any respawn cooldown so everyone spawns on the same tick
		pPlayer->m_RespawnTick = Server()->Tick();
		pPlayer->Respawn();
	}
}

void CGameControllerLTSBlock::OnRoundStart()
{
	CGameControllerBlock::OnRoundStart();

	// restore players who were dead-speced at the end of the previous match
	// m_aPreDeathTeam is TEAM_SPECTATORS for players who never competed
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() != TEAM_SPECTATORS)
			continue;

		int OrigTeam = m_aPreDeathTeam[pPlayer->GetCid()];
		if(OrigTeam != TEAM_RED && OrigTeam != TEAM_BLUE)
			continue;

		pPlayer->m_ForceTeam.m_Tick = 0;
		pPlayer->m_IsDead = false;
		pPlayer->m_KillerId = -1;
		m_aPreDeathTeam[pPlayer->GetCid()] = TEAM_SPECTATORS;

		DoTeamChange(pPlayer, OrigTeam, false);
	}

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		if(pPlayer->GetCharacter())
			continue;
		pPlayer->m_RespawnTick = Server()->Tick();
		pPlayer->Respawn();
	}
}

void CGameControllerLTSBlock::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerBlock::OnPlayerConnect(pPlayer);
	// reset slot so a reconnecting player doesn't inherit the previous occupant's team
	m_aPreDeathTeam[pPlayer->GetCid()] = TEAM_SPECTATORS;
}

void CGameControllerLTSBlock::YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the spectators once the match ends", MsgLen);
}

void CGameControllerLTSBlock::YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the game once the match ends", MsgLen);
}

int CGameControllerLTSBlock::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	// let the parent build the flags, then always re-enable zoom regardless
	// of sv_allow_zoom so players can freely adjust their view distance
	int Flags = CGameControllerBasePvp::SnapGameInfoExFlags(SnappingClient, DDRaceFlags);
	Flags |= GAMEINFOFLAG_ALLOW_ZOOM;
	return Flags;
}

bool CGameControllerLTSBlock::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	if(Team == TEAM_SPECTATORS)
	{
		if(pErrorReason)
			str_copy(pErrorReason, "Spectators are not allowed in this gamemode", ErrorReasonSize);
		return false;
	}
	return CGameControllerBlock::CanJoinTeam(Team, NotThisId, pErrorReason, ErrorReasonSize);
}

void CGameControllerLTSBlock::OnSpecChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Spectating is not allowed in this gamemode.");
}

void CGameControllerLTSBlock::OnPauseChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Spectating is not allowed in this gamemode.");
}

void CGameControllerLTSBlock::OnKillChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Self kill is not allowed in this gamemode.");
}

bool CGameControllerLTSBlock::CanSelfkill(CPlayer *pPlayer, char *pErrorReason, int ErrorReasonSize)
{
	if(pErrorReason)
		str_copy(pErrorReason, "Self kill is not allowed in this gamemode.", ErrorReasonSize);
	return false;
}

REGISTER_GAMEMODE(ltsblock, CGameControllerLTSBlock(pGameServer));

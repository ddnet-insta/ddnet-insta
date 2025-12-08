#include "base_foot.h"

#include <base/system.h>

#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/mapitems_insta.h>
#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <game/server/instagib/entities/foot/foot_pickup.h>
#include <game/server/instagib/entities/foot/foot_projectile.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/player.h>

CGameControllerBaseFoot::CGameControllerBaseFoot(class CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS;
}

CGameControllerBaseFoot::~CGameControllerBaseFoot() = default;

void CGameControllerBaseFoot::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta foot created by ByFox in 2025",
		"This is not a ddnet-insta original mode.",
		"The origin and original creator of the foot gamemode is unknown.",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

int CGameControllerBaseFoot::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	int Flags = CGameControllerBasePvp::SnapGameInfoExFlags(SnappingClient, DDRaceFlags);
	Flags &= ~(GAMEINFOFLAG_ENTITIES_DDRACE);
	Flags &= ~(GAMEINFOFLAG_ENTITIES_RACE);
	Flags &= ~(GAMEINFOFLAG_UNLIMITED_AMMO);
	return Flags;
}

void CGameControllerBaseFoot::Tick()
{
	CGameControllerBasePvp::Tick();
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		if((pChr->GetActiveWeapon() == WEAPON_HAMMER && pChr->GetCore().m_aWeapons[WEAPON_GRENADE].m_Got) || (pChr->m_LoseBallTick && Server()->Tick() >= pChr->m_LoseBallTick))
		{
			const vec2 MouseTarget = vec2(pChr->GetLatestInput().m_TargetX, pChr->GetLatestInput().m_TargetY);
			const vec2 Direction = normalize(MouseTarget);
			const vec2 ProjStartPos = pChr->GetPos() + Direction * pChr->GetProximityRadius() * 0.75f;
			FireGrenade(pChr, Direction, MouseTarget, ProjStartPos);
		}

		pChr->SetArmor((pChr->m_LoseBallTick - Server()->Tick()) * 11 / (Server()->TickSpeed() * 3));
		if(pChr->GetArmor() >= 11)
			pChr->SetArmor(1);
	}

	CFootProjectile *pProjNext = nullptr;
	for(CFootProjectile *pProj = (CFootProjectile *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj; pProj = pProjNext)
	{
		pProjNext = (CFootProjectile *)pProj->TypeNext();

		CPlayer *pPlayer = GetPlayerOrNullptr(pProj->GetOwnerId());
		if(!pPlayer)
			return;

		if(const int GoalTeam = TeamFromGoalTile(pProj->m_LastTouchedTile); GoalTeam != -1)
		{
			GameServer()->m_World.RemoveEntity(pProj);
			if(pPlayer->GetTeam() == GoalTeam)
				OnWrongGoal(pPlayer, pProj);
			else
				OnGoal(pPlayer, pProj);
			OnAnyGoal(pPlayer, pProj);
		}
	}
}

void CGameControllerBaseFoot::OnReset()
{
	CGameControllerBasePvp::OnReset();

	for(int Team = 0; Team < NUM_DDRACE_TEAMS; Team++)
		BallReset(Team, g_Config.m_SvBallRespawnDelay);

	for(const auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;

		if(CCharacter *pChr = pPlayer->GetCharacter(); pChr)
			pChr->Reset();

		pPlayer->m_RespawnTick = Server()->Tick() + 2 * Server()->TickSpeed();
	}

	GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
}

void CGameControllerBaseFoot::OnTeamReset(int Team)
{
	BallReset(Team, g_Config.m_SvBallRespawnDelay);
	for(const auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		const int pTeam = pChr->Team();
		if(pTeam != Team)
			continue;

		// Otherwise, a crash occurs.
		if(!Teams().TeamLocked(pTeam) && pTeam != TEAM_FLOCK)
			Teams().SetTeamLock(pTeam, true);

		pChr->Reset();
		pPlayer->m_RespawnTick = Server()->Tick() + 2 * Server()->TickSpeed();
		GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE, pPlayer->GetCid());
	}
}

void CGameControllerBaseFoot::BallReset(int DDrTeam, int Seconds)
{
	m_aBallTickSpawning[DDrTeam] = Server()->Tick() + Seconds * Server()->TickSpeed();
}

void CGameControllerBaseFoot::OnAnyGoal(CPlayer *pPlayer, CFootProjectile *pProj = nullptr)
{
	if(CCharacter *pChr = pPlayer->GetCharacter())
	{
		GameServer()->CreateExplosion(
			pChr->GetPos(),
			pPlayer->GetCid(),
			WEAPON_GRENADE,
			true,
			pChr->Team(),
			pChr->TeamMask());

		GameServer()->CreateSound(pChr->GetPos(), SOUND_CTF_DROP, pChr->TeamMask());
	}
	OnTeamReset(GameServer()->GetDDRaceTeam(pPlayer->GetCid()));
}

void CGameControllerBaseFoot::OnGoal(CPlayer *pPlayer, CFootProjectile *pProj = nullptr)
{
	pPlayer->IncrementScore();

	char aBuf[512];
	if(pProj && pProj->m_LastTouchedBy != -1 && GetPlayerOrNullptr(pProj->m_LastTouchedBy) && pPlayer->GetTeam() == pProj->m_LastTouchedTeam)
		str_format(aBuf, sizeof(aBuf), "%s scored for the %s with a pass from %s", Server()->ClientName(pProj->m_LastTouchedTeam), pPlayer->GetTeamStr(), Server()->ClientName(pPlayer->GetCid()));
	else
		str_format(aBuf, sizeof(aBuf), "%s scored for the %s team", Server()->ClientName(pProj ? pProj->GetOwnerId() : pPlayer->GetCid()), pPlayer->GetTeamStr());
	GameServer()->SendChatTeam(GameServer()->GetDDRaceTeam(pPlayer->GetCid()), aBuf);

	if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->Team() != TEAM_FLOCK)
		return;

	AddTeamscore(pPlayer->GetTeam(), 1);

	if(IsStatTrack())
		pPlayer->m_Stats.m_Goals += 1;
}

void CGameControllerBaseFoot::OnWrongGoal(CPlayer *pPlayer, CFootProjectile *pProj = nullptr)
{
	pPlayer->DecrementScore();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%s scored for the %s team", Server()->ClientName(pProj ? pProj->GetOwnerId() : pPlayer->GetCid()), pPlayer->GetTeam() == TEAM_RED ? "blue" : "red");
	GameServer()->SendChatTeam(GameServer()->GetDDRaceTeam(pPlayer->GetCid()), aBuf);

	if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->Team() != TEAM_FLOCK)
		return;

	AddTeamscore(!pPlayer->GetTeam(), 1);

	if(IsStatTrack())
		pPlayer->m_Stats.m_OwnGoals += 1;
}

bool CGameControllerBaseFoot::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	if(Weapon != WEAPON_HAMMER)
		return true;

	return false;
}

void CGameControllerBaseFoot::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	CPlayer *pKiller = GetPlayerOrNullptr(From);
	if(!pKiller)
		return;

	if(Weapon == WEAPON_HAMMER && pCharacter->GetCore().m_aWeapons[WEAPON_GRENADE].m_Got)
	{
		pCharacter->LoseBall();
		pKiller->GetCharacter()->PlayerGetBall();
	}
}

void CGameControllerBaseFoot::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	char aBuf[1024];
	str_format(
		aBuf,
		sizeof(aBuf),
		"~~~ all time stats for '%s'",
		pRequestedName);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Goals: %d", pStats->m_Goals);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Deaths: %d", pStats->m_Deaths);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Wins: %d", pStats->m_Wins);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerBaseFoot::OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason)
{
	if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->m_LoseBallTick)
		pPlayer->GetCharacter()->LoseBall();

	CGameControllerBasePvp::OnPlayerDisconnect(pPlayer, pReason);
}

void CGameControllerBaseFoot::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBasePvp::OnCharacterSpawn(pChr);

	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->SetActiveWeapon(WEAPON_HAMMER);
}

int CGameControllerBaseFoot::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	if(pVictim->GetCore().m_aWeapons[WEAPON_GRENADE].m_Got)
		BallReset(pVictim->Team(), g_Config.m_SvBallRespawnDelay);

	return CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
}

bool CGameControllerBaseFoot::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

	if(Index == ENTITY_WEAPON_GRENADE)
	{
		// false positive: CFootPickup constructor itself does GameWorld()->InsertEntity(this)
		// suppress warning about potential memory leak from static analyzer

		// NOLINTBEGIN(clang-analyzer-unix.Malloc)
		CFootPickup *pPickup = new CFootPickup(&GameServer()->m_World, Layer, Number);
		pPickup->m_Pos = Pos;
		for(int &BallTick : m_aBallTickSpawning)
			BallTick = 0;
		// NOLINTEND(clang-analyzer-unix.Malloc)

		return true;
	}

	return CGameControllerBasePvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}

void CGameControllerBaseFoot::SnapDDNetCharacter(int SnappingClient, CCharacter *pChr, CNetObj_DDNetCharacter *pDDNetCharacter)
{
	CGameControllerBasePvp::SnapDDNetCharacter(SnappingClient, pChr, pDDNetCharacter);

	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return;

	CPlayer *pSnapReceiver = GetPlayerOrNullptr(SnappingClient);
	if(!pSnapReceiver)
		return;

	bool IsTeamMate = pChr->GetPlayer()->GetCid() == SnappingClient;
	if(IsTeamPlay() && pChr->GetPlayer()->GetTeam() == pSnapReceiver->GetTeam())
		IsTeamMate = true;
	if(!IsTeamMate && pDDNetCharacter->m_FreezeEnd)
		pDDNetCharacter->m_FreezeEnd = -1;
}

void CGameControllerBaseFoot::FireGrenade(CCharacter *Character, vec2 Direction, vec2 MouseTarget, vec2 ProjStartPos)
{
	if(!Character)
		return;

	CPlayer *pPlayer = Character->GetPlayer();
	if(!pPlayer)
		return;

	if(!pPlayer->GetCharacter())
		return;

	const int GoalTeam = TeamFromGoalTile(GameServer()->Collision()->GetTileIndex(GameServer()->Collision()->GetMapIndex(pPlayer->GetCharacter()->GetPos())));
	if(GoalTeam != -1)
	{
		if(pPlayer->GetTeam() == GoalTeam)
			OnWrongGoal(pPlayer);
		else
			OnGoal(pPlayer);
		OnAnyGoal(pPlayer);
		return;
	}

	CFootProjectile *pProj = new CFootProjectile(
		&GameServer()->m_World,
		WEAPON_GRENADE,
		pPlayer->GetCid(),
		ProjStartPos,
		Direction,
		static_cast<int>(Server()->TickSpeed() * GameServer()->GlobalTuning()->m_GrenadeLifetime),
		false,
		false,
		SOUND_GRENADE_EXPLODE,
		MouseTarget);

	// pack the Projectile and send it to the client Directly
	CNetObj_Projectile p;
	pProj->FillInfo(&p);

	GameServer()->CreateSound(Character->GetPos(), SOUND_GRENADE_FIRE, Character->TeamMask());
	Character->LoseBall();
}

bool CGameControllerBaseFoot::OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos)
{
	if(Weapon == WEAPON_GRENADE)
	{
		if(!Character.m_LoseBallTick && Server()->Tick() >= Character.m_LoseBallTick)
			return true;
		FireGrenade(&Character, Direction, MouseTarget, ProjStartPos);
		return true;
	}

	return false;
}

bool CGameControllerBaseFoot::DoWincheckRound()
{
	if(IsWarmup())
		return false;

	const int ScoreDiff = abs(m_aTeamscore[TEAM_RED] - m_aTeamscore[TEAM_BLUE]);

	const bool ScoreLimitReached = g_Config.m_SvScorelimit > 0 && (m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit);
	const bool TimeLimitReached = g_Config.m_SvTimelimit > 0 && (Server()->Tick() - m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60;

	if(m_SuddenDeath)
	{
		if(ScoreDiff >= g_Config.m_SvSuddenDeathScoreDiff)
		{
			EndRound();
			return true;
		}
		return false;
	}

	if(ScoreLimitReached || TimeLimitReached)
	{
		if(ScoreDiff > g_Config.m_SvScoreDiff)
		{
			EndRound();
			return true;
		}
		m_SuddenDeath = 1;
		GameServer()->SendBroadcast("Sudden Death activated!", -1);

		dbg_msg("game", "Sudden death - scores tied at %d:%d", m_aTeamscore[TEAM_RED], m_aTeamscore[TEAM_BLUE]);
		return false;
	}

	if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
		(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
	{
		if(m_SuddenDeath)
		{
			if(m_aTeamscore[TEAM_RED] / 100 != m_aTeamscore[TEAM_BLUE] / 100)
			{
				EndRound();
				return true;
			}
		}
		else
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
			{
				EndRound();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return false;
}

int CGameControllerBaseFoot::TeamFromGoalTile(int Tile)
{
	switch(Tile)
	{
	case TILE_GOAL_RED:
		return TEAM_RED;
	case TILE_GOAL_BLUE:
		return TEAM_BLUE;
	default:
		return -1;
	}
}

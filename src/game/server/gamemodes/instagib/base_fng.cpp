#include <base/system.h>
#include <base/types.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/mapitems_insta.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/instagib/base_instagib.h>
#include <game/server/instagib/laser_text.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/player.h>

#include "base_fng.h"

CGameControllerBaseFng::CGameControllerBaseFng(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
}

CGameControllerBaseFng::~CGameControllerBaseFng() = default;

int CGameControllerBaseFng::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	int Flags = CGameControllerPvp::SnapGameInfoExFlags(SnappingClient, DDRaceFlags);
	Flags &= ~(GAMEINFOFLAG_ENTITIES_DDNET);
	Flags &= ~(GAMEINFOFLAG_ENTITIES_DDRACE);
	Flags &= ~(GAMEINFOFLAG_ENTITIES_RACE);
	Flags |= GAMEINFOFLAG_ENTITIES_FNG;

	// to make ddnet clients "snd_long_pain" work https://github.com/ddnet-insta/ddnet-insta/issues/298
	Flags |= GAMEINFOFLAG_GAMETYPE_FNG;
	return Flags;
}

void CGameControllerBaseFng::Tick()
{
	CGameControllerInstagib::Tick();

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr || !pChr->IsAlive())
			continue;

		int aSpikeTiles[] = {
			TILE_FNG_SPIKE_RED,
			TILE_FNG_SPIKE_BLUE,
			TILE_FNG_SPIKE_NORMAL,
			TILE_FNG_SPIKE_GOLD,
			TILE_FNG_SPIKE_GREEN,
			TILE_FNG_SPIKE_PURPLE};
		int ClosestTile = 0;
		float ClosestDistance = 1000;

		for(int Spike : aSpikeTiles)
		{
			float Dist = pChr->DistToTouchingTile(Spike);
			if(Dist > 1000)
				continue;
			if(Dist > ClosestDistance)
				continue;

			ClosestDistance = Dist;
			ClosestTile = Spike;
		}

		if(ClosestTile)
			OnSpike(pChr, ClosestTile);
	}
}

void CGameControllerBaseFng::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	CGameControllerInstagib::OnShowStatsAll(pStats, pRequestingPlayer, pRequestedName);

	char aBuf[512];
	str_format(
		aBuf,
		sizeof(aBuf),
		"~~~ all time stats for '%s'",
		pRequestedName);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Points: %d, Wins: %d, Deaths: %d", pStats->m_Points, pStats->m_Wins, pStats->m_Deaths);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	char aAccuracy[512];
	aAccuracy[0] = '\0';
	if(pStats->m_ShotsFired)
		str_format(aAccuracy, sizeof(aAccuracy), " (%.2f%% hit accuracy)", pStats->HitAccuracy());

	str_format(aBuf, sizeof(aBuf), "~ Kills: %d%s", pStats->m_Kills, aAccuracy);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Highest killing spree: %d", pStats->m_BestSpree);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Green spikes: %d, Gold spikes: %d, Purple spikes: %d", pStats->m_GreenSpikes, pStats->m_GoldSpikes, pStats->m_PurpleSpikes);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Highest multi: %d", pStats->m_BestMulti);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	// could use MAX_MU6TIS here but it is too many lines
	// we need a dedicated show multis command for that
	int MaxMultiLines = 6;
	for(int i = 0; i < MaxMultiLines; i++)
	{
		if(!pStats->m_aMultis[i])
			continue;

		str_format(aBuf, sizeof(aBuf), "~  x%d multis %d", i + 2, pStats->m_aMultis[i]);
		if(i + 1 == MaxMultiLines)
			str_append(aBuf, ", see /multis for more");

		SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
	}
}

void CGameControllerBaseFng::OnShowMultis(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	char aBuf[512];
	str_format(
		aBuf,
		sizeof(aBuf),
		"~~~ all time multi stats for '%s'",
		pRequestedName);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Highest multi: %d", pStats->m_BestMulti);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	for(int i = 0; i < MAX_MULTIS; i++)
	{
		if(!pStats->m_aMultis[i])
			continue;

		str_format(aBuf, sizeof(aBuf), "~  x%d multis %d", i + 2, pStats->m_aMultis[i]);
		SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
	}
}

void CGameControllerBaseFng::OnShowSteals(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	char aBuf[512];
	str_format(
		aBuf,
		sizeof(aBuf),
		"~~~ all time stats for '%s'",
		pRequestedName);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Kills stolen from others: %d", pStats->m_StealsFromOthers);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Kills stolen by others: %d", pStats->m_StealsByOthers);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	int TargetId = GetCidByName(pRequestedName);
	if(TargetId < 0 || TargetId >= MAX_CLIENTS)
		return;

	const CPlayer *pPlayer = GameServer()->m_apPlayers[TargetId];

	str_format(aBuf, sizeof(aBuf), "~~~ round stats for '%s'", pRequestedName);
	SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Kills stolen from others: %d", pPlayer->m_Stats.m_StealsFromOthers);
	SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Kills stolen by others: %d", pPlayer->m_Stats.m_StealsByOthers);
	SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerBaseFng::OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason)
{
	for(CPlayer *pOther : GameServer()->m_apPlayers)
	{
		if(!pOther)
			continue;

		// if someone leaves a frozen tee behing on disconnect
		// and a teammates spikes him this is not considered a steal
		if(pOther->m_OriginalFreezerId == pPlayer->GetCid())
			pOther->m_OriginalFreezerId = -1;
	}

	CGameControllerInstagib::OnPlayerDisconnect(pPlayer, pReason);
}

void CGameControllerBaseFng::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerInstagib::OnPlayerConnect(pPlayer);
}

void CGameControllerBaseFng::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstagib::OnCharacterSpawn(pChr);

	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
}

int CGameControllerBaseFng::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerInstagib::OnCharacterDeath(pVictim, pKiller, WeaponId);
	return 0;
}

bool CGameControllerBaseFng::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
	return false;
}

void CGameControllerBaseFng::OnWrongSpike(class CPlayer *pPlayer, int RemoveScore)
{
	if(IsStatTrack())
		pPlayer->m_Stats.m_WrongSpikes++;
	pPlayer->AddScore(RemoveScore - 1);
	CCharacter *pChr = pPlayer->GetCharacter();
	// this means you can selfkill before the wrong spike hits
	// to bypass getting frozen
	// but that seems fine
	if(!pChr)
		return;

	pPlayer->UpdateLastToucher(-1);
	if(g_Config.m_SvWrongSpikeFreeze)
		pChr->Freeze(g_Config.m_SvWrongSpikeFreeze);
}

void CGameControllerBaseFng::OnSpike(class CCharacter *pChr, int SpikeTile)
{
	if(!pChr->m_FreezeTime)
	{
		pChr->Die(pChr->GetPlayer()->GetCid(), WEAPON_WORLD);
		return;
	}

	CPlayer *pKiller = nullptr;
	const int LastToucherId = pChr->GetPlayer()->m_LastToucherId;
	if(LastToucherId >= 0 && LastToucherId < MAX_CLIENTS)
		pKiller = GameServer()->m_apPlayers[LastToucherId];

	if(pKiller)
	{
		switch(SpikeTile)
		{
		case TILE_FNG_SPIKE_NORMAL:
			UpdateScoresAndDisplayPoints(pKiller, g_Config.m_SvPlayerScoreSpikeNormal, g_Config.m_SvTeamScoreSpikeNormal);
			break;
		case TILE_FNG_SPIKE_GOLD:
			if(IsStatTrack())
				pKiller->m_Stats.m_GoldSpikes++;
			UpdateScoresAndDisplayPoints(pKiller, g_Config.m_SvPlayerScoreSpikeGold, g_Config.m_SvTeamScoreSpikeGold);
			break;
		case TILE_FNG_SPIKE_GREEN:
			if(IsStatTrack())
				pKiller->m_Stats.m_GreenSpikes++;
			UpdateScoresAndDisplayPoints(pKiller, g_Config.m_SvPlayerScoreSpikeGreen, g_Config.m_SvTeamScoreSpikeGreen);
			break;
		case TILE_FNG_SPIKE_PURPLE:
			if(IsStatTrack())
				pKiller->m_Stats.m_PurpleSpikes++;
			UpdateScoresAndDisplayPoints(pKiller, g_Config.m_SvPlayerScoreSpikePurple, g_Config.m_SvTeamScoreSpikePurple);
			break;
		case TILE_FNG_SPIKE_RED:
			if(pKiller->GetTeam() == TEAM_RED || !IsTeamPlay())
				UpdateScoresAndDisplayPoints(pKiller, g_Config.m_SvPlayerScoreSpikeTeam, g_Config.m_SvTeamScoreSpikeTeam);
			else
				OnWrongSpike(pKiller, -g_Config.m_SvPlayerScoreSpikeTeam);
			break;
		case TILE_FNG_SPIKE_BLUE:
			if(pKiller->GetTeam() == TEAM_BLUE || !IsTeamPlay())
				UpdateScoresAndDisplayPoints(pKiller, g_Config.m_SvPlayerScoreSpikeTeam, g_Config.m_SvTeamScoreSpikeTeam);
			else
				OnWrongSpike(pKiller, -g_Config.m_SvPlayerScoreSpikeTeam);
			break;
		default:
			break;
		}

		// yes you can multi wrong spikes
		pKiller->m_LastKillTime = pKiller->HandleMulti();

		// check for steal
		if(pChr->GetPlayer()->m_OriginalFreezerId != -1)
		{
			CPlayer *pOriginalFreezer = GameServer()->m_apPlayers[pChr->GetPlayer()->m_OriginalFreezerId];
			if(pOriginalFreezer && pOriginalFreezer != pKiller)
			{
				if(g_Config.m_SvAnnounceSteals)
				{
					char aBuf[512];
					str_format(
						aBuf,
						sizeof(aBuf),
						"'%s' kill was stolen by '%s'.",
						Server()->ClientName(pOriginalFreezer->GetCid()),
						Server()->ClientName(pKiller->GetCid()));
					GameServer()->SendChat(-1, TEAM_ALL, aBuf);
				}

				if(IsStatTrack())
				{
					pKiller->m_Stats.m_StealsFromOthers++;
					pOriginalFreezer->m_Stats.m_StealsByOthers++;
				}
			}
		}

		// do sacrifice flag sound
		if(g_Config.m_SvFngSpikeSound == 1)
		{
			CClientMask Mask = CClientMask().set(pKiller->GetCid());
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(!GameServer()->m_apPlayers[i])
					continue;

				if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->SpectatorId() == pKiller->GetCid())
					Mask.set(i);
			}
			GameServer()->CreateSound(pKiller->m_ViewPos, SOUND_CTF_CAPTURE, Mask);
		}
		else if(g_Config.m_SvFngSpikeSound == 2)
		{
			CClientMask Mask = pChr->TeamMask();
			Mask.reset(pKiller->GetCid());
			GameServer()->CreateSound(pChr->GetPos(), SOUND_CTF_GRAB_PL, Mask);
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE, pKiller->GetCid());
		}

		OnKill(pChr->GetPlayer(), pKiller, WEAPON_WORLD);
	}

	if(LastToucherId == -1)
		pChr->Die(pChr->GetPlayer()->GetCid(), WEAPON_WORLD);
	else
		pChr->Die(LastToucherId, WEAPON_NINJA);
}

inline void CGameControllerBaseFng::UpdateScoresAndDisplayPoints(CPlayer *pKiller, int PlayerScore, int TeamScore)
{
	if(!pKiller)
		return;
	// The player already gets one score point
	// in CGameControllerPvp::OnCharacterDeath()
	pKiller->AddScore(PlayerScore - 1);
	AddTeamscore(pKiller->GetTeam(), TeamScore);
	if(pKiller->IsPlaying()) // NOLINT(clang-analyzer-unix.Malloc)
		MakeLaserTextPoints(pKiller->GetCharacter()->GetPos(), PlayerScore, 3); // NOLINT(clang-analyzer-unix.Malloc)
}

void CGameControllerBaseFng::SnapDDNetCharacter(int SnappingClient, CCharacter *pChr, CNetObj_DDNetCharacter *pDDNetCharacter)
{
	CGameControllerInstagib::SnapDDNetCharacter(SnappingClient, pChr, pDDNetCharacter);

	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return;

	CPlayer *pSnapReceiver = GameServer()->m_apPlayers[SnappingClient];
	if(!pSnapReceiver)
		return;

	bool IsTeamMate = pChr->GetPlayer()->GetCid() == SnappingClient;
	if(IsTeamPlay() && pChr->GetPlayer()->GetTeam() == pSnapReceiver->GetTeam())
		IsTeamMate = true;
	if(!IsTeamMate && pDDNetCharacter->m_FreezeEnd)
		pDDNetCharacter->m_FreezeEnd = -1;
}

CClientMask CGameControllerBaseFng::FreezeDamageIndicatorMask(class CCharacter *pChr)
{
	CClientMask Mask = pChr->TeamMask() & GameServer()->ClientsMaskExcludeClientVersionAndHigher(VERSION_DDNET_NEW_HUD);
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() == pChr->GetPlayer()->GetTeam() && GameServer()->m_pController->IsTeamPlay())
			continue;
		if(pPlayer->GetCid() == pChr->GetPlayer()->GetCid())
			continue;

		Mask.reset(pPlayer->GetCid());
	}
	return Mask;
}

bool CGameControllerBaseFng::OnSelfkill(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;
	if(!pChr->m_FreezeTime)
		return false;

	GameServer()->SendChatTarget(ClientId, "You can't kill while being frozen");
	return true;
}

// called after spam protection on client team join request
bool CGameControllerBaseFng::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
	if(pPlayer)
	{
		CCharacter *pChr = pPlayer->GetCharacter();
		if(pChr && pChr->m_FreezeTime)
		{
			str_copy(pErrorReason, "You can't join spectators while being frozen", ErrorReasonSize);
			return false;
		}
	}

	return CGameControllerInstagib::CanJoinTeam(Team, NotThisId, pErrorReason, ErrorReasonSize);
}

bool CGameControllerBaseFng::OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim)
{
	// do not track wallshots on frozen tees
	if(pVictim->m_FreezeTime)
		return true;
	return CGameControllerInstagib::OnLaserHit(Bounces, From, Weapon, pVictim);
}

bool CGameControllerBaseFng::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	if(pCharacter->m_FreezeTime)
		return true;
	if(Weapon == WEAPON_HAMMER)
		return true;

	return CGameControllerInstagib::SkipDamage(Dmg, From, Weapon, pCharacter, ApplyForce);
}

// warning this does not call the base pvp take damage method
// so it has to reimplement all the relevant functionality
bool CGameControllerBaseFng::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	OnAnyDamage(Force, Dmg, From, Weapon, &Character);

	bool ApplyForce = true;
	if(SkipDamage(Dmg, From, Weapon, &Character, ApplyForce))
	{
		Dmg = 0;
		return !ApplyForce;
	}

	OnAppliedDamage(Dmg, From, Weapon, &Character);

	CPlayer *pKiller = GetPlayerOrNullptr(From);

	// do scoring
	if(pKiller)
	{
		if(IsTeamPlay() && Character.GetPlayer()->GetTeam() == pKiller->GetTeam())
		{
			pKiller->DecrementScore(); // teamkill
		}
		else
		{
			pKiller->IncrementScore(); // normal kill
			AddTeamscore(pKiller->GetTeam(), 1);
		}
	}

	if(IsStatTrack())
	{
		Character.GetPlayer()->m_Stats.m_GotFrozen++;
	}

	// kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = From;
	Msg.m_Victim = Character.GetPlayer()->GetCid();
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = 0;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

	GameServer()->CreateDeath(Character.m_Pos, Character.GetPlayer()->GetCid(), Character.TeamMask());

	Character.GetPlayer()->UpdateLastToucher(From);
	Character.GetPlayer()->m_OriginalFreezerId = From;
	Character.Freeze(g_Config.m_SvHitFreezeDelay);
	return false;
}

void CGameControllerBaseFng::Snap(int SnappingClient)
{
	CGameControllerInstagib::Snap(SnappingClient);
}

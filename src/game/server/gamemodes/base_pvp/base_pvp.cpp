#include "base_pvp.h"

#include <base/log.h>
#include <base/system.h>

#include <engine/server/server.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/race_state.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/insta_core/insta_core.h>
#include <game/server/instagib/entities/ddnet_pvp/vanilla_projectile.h>
#include <game/server/instagib/entities/flag.h>
#include <game/server/instagib/enums.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/structs.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/server/teams.h>
#include <game/teamscore.h>

#include <cstdint>

CGameControllerBasePvp::CGameControllerBasePvp(class CGameContext *pGameServer) :
	CGameControllerInstaCore(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;

	GameServer()->GlobalTuning()->Set("gun_curvature", 1.25f);
	GameServer()->GlobalTuning()->Set("gun_speed", 2200);
	GameServer()->GlobalTuning()->Set("shotgun_curvature", 1.25f);
	GameServer()->GlobalTuning()->Set("shotgun_speed", 2750);
	GameServer()->GlobalTuning()->Set("shotgun_speeddiff", 0.8f);

	log_info("ddnet-insta", "connecting to database ...");
	// set the stats table to the gametype name in all lowercase
	// if you want to track stats in a sql database for that gametype
	m_pStatsTable = "";
	m_pSqlStats = new CSqlStats(GameServer(), ((CServer *)Server())->DbPool());
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);

	// https://github.com/ddnet-insta/ddnet-insta/issues/253
	// always umute spectators on map change or "reload" command
	//
	// this constructor is not called on "restart" commands
	if(g_Config.m_SvTournamentChatSmart)
		g_Config.m_SvTournamentChat = 0;

	m_UnbalancedTick = TBALANCE_OK;
}

CGameControllerBasePvp::~CGameControllerBasePvp()
{
	// TODO: we have to make sure to block all operations and save everything if sv_gametype is switched
	//       there should be no data loss no matter in which state and how often the controller is recreated
	//
	//       this also has to save player sprees that were not ended yet!
	dbg_msg("ddnet-insta", "cleaning up database connection ...");
	if(m_pSqlStats)
	{
		delete m_pSqlStats;
		m_pSqlStats = nullptr;
	}

	if(m_pExtraColumns)
	{
		delete m_pExtraColumns;
		m_pExtraColumns = nullptr;
	}
}

void CGameControllerBasePvp::OnInit()
{
	CGameControllerInstaCore::OnInit();

	if(GameFlags() & GAMEFLAG_FLAGS)
	{
		m_pSqlStats->CreateFastcapTable();
	}
}

void CGameControllerBasePvp::OnRoundStart()
{
	log_debug(
		"ddnet-insta",
		"new round start! Current game state: %s",
		GameStateToStr(GameState()));

	int StartGameState = GameState();

	m_GameStartTick = Server()->Tick();
	SetGameState(IGS_GAME_RUNNING);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;
	m_aTeamscore[TEAM_RED] = 0;
	m_aTeamscore[TEAM_BLUE] = 0;

	// only auto start round if we are in casual mode and there is no tournament running
	// otherwise set infinite warmup and wait for !restart
	if(StartGameState == IGS_END_ROUND && (!g_Config.m_SvCasualRounds || g_Config.m_SvTournament))
	{
		SendChat(-1, TEAM_ALL, "Starting warmup phase. Call a restart vote to start a new game.");
		SetGameState(IGS_WARMUP_GAME, TIMER_INFINITE);
	}
	else
	{
		SetGameState(IGS_START_COUNTDOWN_ROUND_START);
	}

	// yes the config says round end and the code is round start
	// that is because every round start means there was a round end before
	// so it is correct
	//
	// round end is too early because then players do not see the final scoreboard
	if(g_Config.m_SvRedirectAndShutdownOnRoundEnd)
	{
		for(int i = 0; i < Server()->MaxClients(); i++)
			if(Server()->ClientIngame(i))
				Server()->RedirectClient(i, g_Config.m_SvRedirectAndShutdownOnRoundEnd);

		// add a 3 second delay to make sure the server can fully finish the round end
		// everything is shutdown and saved correctly
		//
		// and also give the clients some time to receive the redirect message
		// in case there is some network overload or hiccups
		m_TicksUntilShutdown = Server()->TickSpeed() * 3;
	}

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		RoundInitPlayer(pPlayer);
	}
}

void CGameControllerBasePvp::OnRoundEnd()
{
	CGameControllerInstaCore::OnRoundEnd();

	if(g_Config.m_SvTournamentChatSmart)
	{
		if(g_Config.m_SvTournamentChat)
			GameServer()->SendChat(-1, TEAM_ALL, "Spectators can use public chat again");
		g_Config.m_SvTournamentChat = 0;
	}

	PublishRoundEndStats();
	if(g_Config.m_SvPrintRoundStats)
		SendRoundTopMessage(-1);

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(g_Config.m_SvKillingspreeResetOnRoundEnd)
		{
			// TODO: it is a bit weird that it says:
			//         x's spree was ended by x
			//       it should instead say something like:
			//         x's spree was ended by game/round
			EndSpree(pPlayer, pPlayer);
		}

		if(m_pStatsTable[0] != '\0')
			SaveStatsOnRoundEnd(pPlayer);
	}
}

int CGameControllerBasePvp::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	int Flags =
		GAMEINFOFLAG_PREDICT_VANILLA | // ddnet-insta
		GAMEINFOFLAG_ENTITIES_VANILLA | // ddnet-insta
		GAMEINFOFLAG_BUG_VANILLA_BOUNCE | // ddnet-insta
		GAMEINFOFLAG_GAMETYPE_VANILLA | // ddnet-insta
		/* GAMEINFOFLAG_TIMESCORE | */ // ddnet-insta
		/* GAMEINFOFLAG_GAMETYPE_RACE | */ // ddnet-insta
		/* GAMEINFOFLAG_GAMETYPE_DDRACE | */ // ddnet-insta
		/* GAMEINFOFLAG_GAMETYPE_DDNET | */ // ddnet-insta
		GAMEINFOFLAG_UNLIMITED_AMMO |
		GAMEINFOFLAG_RACE_RECORD_MESSAGE |
		GAMEINFOFLAG_ALLOW_EYE_WHEEL |
		/* GAMEINFOFLAG_ALLOW_HOOK_COLL | */ // https://github.com/ddnet-insta/ddnet-insta/issues/195
		GAMEINFOFLAG_ALLOW_ZOOM |
		GAMEINFOFLAG_BUG_DDRACE_GHOST |
		/* GAMEINFOFLAG_BUG_DDRACE_INPUT | */ // https://github.com/ddnet-insta/ddnet-insta/issues/161
		GAMEINFOFLAG_PREDICT_DDRACE |
		GAMEINFOFLAG_PREDICT_DDRACE_TILES |
		GAMEINFOFLAG_ENTITIES_DDNET |
		GAMEINFOFLAG_ENTITIES_DDRACE |
		GAMEINFOFLAG_ENTITIES_RACE |
		GAMEINFOFLAG_RACE;
	if(!g_Config.m_SvAllowZoom) //ddnet-insta
		Flags &= ~(GAMEINFOFLAG_ALLOW_ZOOM);

	// ddnet clients do not predict sv_old_laser correctly
	// https://github.com/ddnet/ddnet/issues/7589
	if(g_Config.m_SvOldLaser)
		Flags &= ~(GAMEINFOFLAG_PREDICT_DDRACE);

	return Flags;
}

int CGameControllerBasePvp::SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags)
{
	return GAMEINFOFLAG2_HUD_AMMO | GAMEINFOFLAG2_HUD_HEALTH_ARMOR;
}

int CGameControllerBasePvp::SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore)
{
	CPlayer *pSnapReceiver = GetPlayerOrNullptr(SnappingClient);
	if(!pSnapReceiver)
		return DDRaceScore;

	int Score = pPlayer->m_Score.value_or(0);

	// always force display round score if the game ended
	// otherwise you can not see who actually won
	//
	// in zCatch you do win by score
	// and you also do make any points during round
	// so we just keep display whatever we displayed during the round
	// https://github.com/ddnet-insta/ddnet-insta/issues/233
	// TODO: once there are other non points winning gametypes
	//       we should introduce something like IsLmsGameType()
	//       and use that here
	//       but that really depends on how these gametypes
	//       do scoring and give points
	if(GameState() == IGS_END_ROUND && !IsZcatchGameType())
		return Score;

	switch(pSnapReceiver->m_DisplayScore)
	{
	case EDisplayScore::NUM_SCORES:
	case EDisplayScore::ROUND_POINTS:
		return Score;
	case EDisplayScore::POINTS:
		return Score + pPlayer->m_SavedStats.m_Points;
	case EDisplayScore::SPREE:
		return pPlayer->m_SavedStats.m_BestSpree;
	case EDisplayScore::CURRENT_SPREE:
		return pPlayer->Spree();
	case EDisplayScore::WIN_POINTS:
		return pPlayer->m_SavedStats.m_WinPoints;
	case EDisplayScore::WINS:
		return pPlayer->m_SavedStats.m_Wins;
	case EDisplayScore::KILLS:
		return pPlayer->Kills() + pPlayer->m_SavedStats.m_Kills;
	case EDisplayScore::ROUND_KILLS:
		return pPlayer->Kills();
	};

	return Score;
}

bool CGameControllerBasePvp::IsGrenadeGameType() const
{
	// TODO: this should be done with some cleaner spawnweapons/available weapons enum flag thing
	if(IsZcatchGameType())
	{
		return m_SpawnWeapons == SPAWN_WEAPON_GRENADE;
	}
	return IsVanillaGameType() || m_pGameType[0] == 'g';
}

void CGameControllerBasePvp::OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks)
{
	if(!pFlag)
		return;
	if(!pFlag->m_pCarrier)
		return;
	if(TimeTicks <= 0)
		return;

	int ClientId = pFlag->m_pCarrier->GetPlayer()->GetCid();

	// TODO: find a better way to check if there is a grenade or not
	bool Grenade = IsGrenadeGameType();

	char aTimestamp[TIMESTAMP_STR_LENGTH];
	str_timestamp_format(aTimestamp, sizeof(aTimestamp), FORMAT_SPACE); // 2019-04-02 19:41:58

	m_pSqlStats->SaveFastcap(ClientId, TimeTicks, aTimestamp, Grenade, IsStatTrack());
}

bool CGameControllerBasePvp::ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos)
{
	if(!g_Config.m_SvStrictSnapDistance)
		return false;
	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[SnappingClient];
	const bool IsSpectator = pPlayer->GetTeam() == TEAM_SPECTATORS;
	const bool ForceDefaultView = !g_Config.m_SvAllowZoom && !IsSpectator;

	if(!ForceDefaultView && pPlayer->m_ShowAll)
		return false;

	// ddnet-insta: snap default if player is ingame
	vec2 &ShowDistance = pPlayer->m_ShowDistance;

	// https://github.com/teeworlds/teeworlds/blob/93f5bf632a3859e97d527fc93a26b6dced767fbc/src/game/server/entity.cpp#L44
	if(ForceDefaultView)
		ShowDistance = vec2(1000, 800);

	float dx = pPlayer->m_ViewPos.x - CheckPos.x;
	if(absolute(dx) > ShowDistance.x)
		return true;

	float dy = pPlayer->m_ViewPos.y - CheckPos.y;
	return absolute(dy) > ShowDistance.y;
}

bool CGameControllerBasePvp::ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos)
{
	if(!g_Config.m_SvStrictSnapDistance)
		return false;
	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[SnappingClient];
	const bool IsSpectator = pPlayer->GetTeam() == TEAM_SPECTATORS;
	const bool ForceDefaultView = !g_Config.m_SvAllowZoom && !IsSpectator;

	if(!ForceDefaultView && pPlayer->m_ShowAll)
		return false;

	vec2 &ViewPos = pPlayer->m_ViewPos;
	vec2 &ShowDistance = pPlayer->m_ShowDistance;

	// https://github.com/teeworlds/teeworlds/blob/93f5bf632a3859e97d527fc93a26b6dced767fbc/src/game/server/entity.cpp#L44
	if(ForceDefaultView)
		ShowDistance = vec2(1000, 800);

	vec2 DistanceToLine, ClosestPoint;
	if(closest_point_on_line(StartPos, EndPos, ViewPos, ClosestPoint))
	{
		DistanceToLine = ViewPos - ClosestPoint;
	}
	else
	{
		// No line section was passed but two equal points
		DistanceToLine = ViewPos - StartPos;
	}
	float ClippDistance = maximum(ShowDistance.x, ShowDistance.y);
	return (absolute(DistanceToLine.x) > ClippDistance || absolute(DistanceToLine.y) > ClippDistance);
}

void CGameControllerBasePvp::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	char aBuf[1024];
	str_format(
		aBuf,
		sizeof(aBuf),
		"~~~ all time stats for '%s'",
		pRequestedName);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Points: %d", pStats->m_Points);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	char aAccuracy[512];
	aAccuracy[0] = '\0';
	if(pStats->m_ShotsFired)
		str_format(aAccuracy, sizeof(aAccuracy), " (%.2f%% hit accuracy)", pStats->HitAccuracy());

	str_format(aBuf, sizeof(aBuf), "~ Kills: %d%s", pStats->m_Kills, aAccuracy);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Deaths: %d", pStats->m_Deaths);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Wins: %d", pStats->m_Wins);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Highest killing spree: %d", pStats->m_BestSpree);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerBasePvp::OnShowRank(int Rank, int RankedScore, const char *pRankType, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	char aBuf[1024];
	str_format(
		aBuf,
		sizeof(aBuf),
		"%d. '%s' %s: %d, requested by '%s'",
		Rank, pRequestedName, pRankType, RankedScore, Server()->ClientName(pRequestingPlayer->GetCid()));

	if(AllowPublicChat(pRequestingPlayer))
	{
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		return;
	}

	int Team = pRequestingPlayer->GetTeam();
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() != Team)
			continue;

		SendChatTarget(pPlayer->GetCid(), aBuf);
	}
}

bool CGameControllerBasePvp::IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage)
{
	if(pMessage && SizeOfMessage)
		pMessage[0] = '\0';

	// you can only win on round end
	if(GameState() != IGS_END_ROUND)
		return false;
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
		return false;

	return HasWinningScore(pPlayer);
}

bool CGameControllerBasePvp::IsLoser(const CPlayer *pPlayer)
{
	// TODO: we could relax this a bit
	//       and not count every disconnect during a running round as loss
	//       for example when you or your team is leading or its currently a draw
	//       then it could be neither a win or a loss to quit mid round
	//
	//       the draw case also covers quitting a few seconds after round start
	//       which is common for users who connected to the wrong server
	//       or users that quit after the previous round ended
	if(GameState() != IGS_END_ROUND)
		return true;
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
		return false;

	return !IsWinner(pPlayer, 0, 0);
}

bool CGameControllerBasePvp::IsStatTrack(char *pReason, int SizeOfReason)
{
	if(pReason)
		pReason[0] = '\0';

	if(g_Config.m_SvAlwaysTrackStats)
	{
		if(g_Config.m_SvDebugStats)
			log_debug("stats", "tracking stats no matter what because sv_always_track_stats is set");
		return true;
	}

	if(IsWarmup())
	{
		if(pReason)
			str_copy(pReason, "warmup", SizeOfReason);
		return false;
	}

	int MinPlayers = IsTeamPlay() ? 3 : 2;
	int Count = NumConnectedIps();
	bool Track = Count >= MinPlayers;
	if(g_Config.m_SvDebugStats)
		log_debug("stats", "connected unique ips=%d (%d+ needed to track) tracking=%d", Count, MinPlayers, Track);

	if(!Track)
	{
		if(pReason)
			str_copy(pReason, "not enough players", SizeOfReason);
	}

	return Track;
}

void CGameControllerBasePvp::SaveStatsOnRoundEnd(CPlayer *pPlayer)
{
	char aMsg[512] = {0};
	bool Won = IsWinner(pPlayer, aMsg, sizeof(aMsg));
	bool Lost = IsLoser(pPlayer);
	// dbg_msg("stats", "winner=%d loser=%d msg=%s name: %s", Won, Lost, aMsg, Server()->ClientName(pPlayer->GetCid()));
	if(aMsg[0])
		GameServer()->SendChatTarget(pPlayer->GetCid(), aMsg);

	dbg_msg("stats", "saving round stats of player '%s' win=%d loss=%d msg='%s'", Server()->ClientName(pPlayer->GetCid()), Won, Lost, aMsg);

	// the spree can not be incremented if stat track is off
	// but the best spree will be counted even if it is off
	// this ensures that the spree of a player counts that
	// dominated the entire server into rq and never died
	if(pPlayer->Spree() > pPlayer->m_Stats.m_BestSpree)
	{
		log_info("stats", "player '%s' has a spree of %d kills that was not tracked (force tracking it now)", Server()->ClientName(pPlayer->GetCid()), pPlayer->Spree());
		log_info("stats", "player '%s' currently has %d tracked kills", Server()->ClientName(pPlayer->GetCid()), pPlayer->m_Stats.m_Kills);
		pPlayer->m_Stats.m_BestSpree = pPlayer->Spree();
	}
	if(IsStatTrack())
	{
		if(Won)
		{
			pPlayer->m_Stats.m_Wins++;
			pPlayer->m_Stats.m_Points++;
			pPlayer->m_Stats.m_WinPoints += WinPointsForWin(pPlayer);
		}
		if(Lost)
			pPlayer->m_Stats.m_Losses++;
	}

	m_pSqlStats->SaveRoundStats(Server()->ClientName(pPlayer->GetCid()), StatsTable(), &pPlayer->m_Stats);

	// instead of doing a db write and read for ALL players
	// on round end we manually sum up the stats for save servers
	// this means that if someone else is using the same name on
	// another server the stats will be outdated
	// but that is fine
	//
	// saved stats are a gimmic not a source of truth
	pPlayer->m_SavedStats.Merge(&pPlayer->m_Stats);
	if(m_pExtraColumns)
		m_pExtraColumns->MergeStats(&pPlayer->m_SavedStats, &pPlayer->m_Stats);

	pPlayer->ResetStats();
}

void CGameControllerBasePvp::SaveStatsOnDisconnect(CPlayer *pPlayer)
{
	if(!pPlayer)
		return;
	if(!pPlayer->m_Stats.HasValues(m_pExtraColumns))
		return;

	// the spree can not be incremented if stat track is off
	// but the best spree will be counted even if it is off
	// this ensures that the spree of a player counts that
	// dominated the entire server into rq and never died
	if(pPlayer->Spree() > pPlayer->m_Stats.m_BestSpree)
		pPlayer->m_Stats.m_BestSpree = pPlayer->Spree();

	// rage quit during a round counts as loss
	bool CountAsLoss = true;
	const char *pLossReason = "rage quit";

	// unless there are not enough players to track stats
	// fast capping alone on a server should never increment losses
	if(!IsStatTrack())
	{
		CountAsLoss = false;
		pLossReason = "stat track is off";
	}

	// unless you are just a spectator
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		int SpectatorTicks = Server()->Tick() - pPlayer->m_LastSetTeam;
		int SpectatorSeconds = SpectatorTicks / Server()->TickSpeed();
		if(SpectatorSeconds > 60)
		{
			CountAsLoss = false;
			pLossReason = "player is spectator";
		}
		// dbg_msg("stats", "spectator since %d seconds", SpectatorSeconds);
	}

	// if the quitting player was about to win
	// it does not count as a loss either
	if(HasWinningScore(pPlayer))
	{
		CountAsLoss = false;
		pLossReason = "player has winning score";
	}

	// require at least one death to count aborting a game as loss
	if(!pPlayer->m_Stats.m_Deaths)
	{
		CountAsLoss = false;
		pLossReason = "player never died";
	}

	int RoundTicks = Server()->Tick() - m_GameStartTick;
	int ConnectedTicks = Server()->Tick() - pPlayer->m_JoinTick;
	int RoundSeconds = RoundTicks / Server()->TickSpeed();
	int ConnectedSeconds = ConnectedTicks / Server()->TickSpeed();

	// dbg_msg(
	// 	"disconnect",
	// 	"round_ticks=%d (%ds)   connected_ticks=%d (%ds)",
	// 	RoundTicks,
	// 	RoundSeconds,
	// 	ConnectedTicks,
	// 	ConnectedSeconds);

	// the player has to be playing in that round for at least one minute
	// for it to count as a loss
	//
	// this protects from reconnecting stat griefers
	// and also makes sure that casual short connects don't count as loss
	if(RoundSeconds < 60 || ConnectedSeconds < 60)
	{
		CountAsLoss = false;
		pLossReason = "player did not play long enough";
	}

	if(CountAsLoss)
		pPlayer->m_Stats.m_Losses++;

	dbg_msg("sql", "saving stats of disconnecting player '%s' CountAsLoss=%d (%s)", Server()->ClientName(pPlayer->GetCid()), CountAsLoss, pLossReason);
	m_pSqlStats->SaveRoundStats(Server()->ClientName(pPlayer->GetCid()), StatsTable(), &pPlayer->m_Stats);
}

int CGameControllerBasePvp::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	CGameControllerInstaCore::OnCharacterDeath(pVictim, pKiller, Weapon);

	// do scoring
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;

	// never count score or win rounds in ddrace teams
	if(GameServer()->GetDDRaceTeam(pKiller->GetCid()))
		return 0;
	if(GameServer()->GetDDRaceTeam(pVictim->GetPlayer()->GetCid()))
		return 0;

	const bool SelfKill = pKiller == pVictim->GetPlayer();
	const bool SuicideOrWorld = Weapon == WEAPON_SELF || Weapon == WEAPON_WORLD || SelfKill;

	if(SuicideOrWorld)
	{
		if(g_Config.m_SvSuicidePenalty)
			pVictim->GetPlayer()->DecrementScore();
	}
	else
	{
		if(IsTeamPlay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
			pKiller->DecrementScore(); // teamkill
		else
			pKiller->IncrementScore(); // normal kill
	}

	// update spectator modes for dead players in survival
	// if(m_GameFlags&GAMEFLAG_SURVIVAL)
	// {
	// 	for(int i = 0; i < MAX_CLIENTS; ++i)
	// 		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_DeadSpecMode)
	// 			GameServer()->m_apPlayers[i]->UpdateDeadSpecMode();
	// }

	// selfkill is no kill
	if(!SelfKill)
		pKiller->AddKill();
	// but selfkill is a death
	pVictim->GetPlayer()->AddDeath();

	if(pKiller && pVictim)
	{
		if(pKiller->GetCharacter() && pKiller != pVictim->GetPlayer())
		{
			AddSpree(pKiller);
		}

		bool IsSpreeEnd = true;
		// only getting spiked can end sprees in fng
		if(IsFngGameType() && SuicideOrWorld)
			IsSpreeEnd = false;
		if(IsSpreeEnd)
			EndSpree(pVictim->GetPlayer(), pKiller);
	}
	return 0;
}

void CGameControllerBasePvp::CheckForceUnpauseGame()
{
	if(!Config()->m_SvForceReadyAll)
		return;
	if(m_GamePauseStartTime == -1)
		return;

	const int MinutesPaused = ((time_get() - m_GamePauseStartTime) / time_freq()) / 60;
	// dbg_msg("insta", "paused since %d minutes", MinutesPaused);

	// const int SecondsPausedDebug = (time_get() - m_GamePauseStartTime) / time_freq();
	// dbg_msg("insta", "paused since %d seconds [DEBUG ONLY REMOVE THIS]", SecondsPausedDebug);

	const int64_t ForceUnpauseTime = m_GamePauseStartTime + (Config()->m_SvForceReadyAll * 60 * time_freq());
	// dbg_msg("insta", "    ForceUnpauseTime=%ld secs=%ld secs_diff_now=%ld [DEBUG ONLY REMOVE THIS]", ForceUnpauseTime, ForceUnpauseTime / time_freq(), (time_get() - ForceUnpauseTime) / time_freq());
	// dbg_msg("insta", "m_GamePauseStartTime=%ld secs=%ld secs_diff_now=%ld [DEBUG ONLY REMOVE THIS]", m_GamePauseStartTime, m_GamePauseStartTime / time_freq(), (time_get() - m_GamePauseStartTime) / time_freq());
	const int SecondsUntilForceUnpause = (ForceUnpauseTime - time_get()) / time_freq();
	// dbg_msg("insta", "seconds until force unpause %d [DEBUG ONLY REMOVE THIS]", SecondsUntilForceUnpause);

	char aBuf[512];
	aBuf[0] = '\0';
	if(SecondsUntilForceUnpause == 60)
		str_copy(aBuf, "Game will be force unpaused in 1 minute.");
	else if(SecondsUntilForceUnpause == 10)
		str_copy(aBuf, "Game will be force unpaused in 10 seconds.");
	if(aBuf[0])
	{
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	}

	if(MinutesPaused >= Config()->m_SvForceReadyAll)
	{
		GameServer()->SendChat(-1, TEAM_ALL, "Force unpaused the game because the maximum pause time was reached.");
		SetPlayersReadyState(true);
		CheckReadyStates();
	}
}

void CGameControllerBasePvp::Tick()
{
	CGameControllerInstaCore::Tick();

	if(Config()->m_SvPlayerReadyMode && GameServer()->m_World.m_Paused)
	{
		if(Server()->Tick() % (Server()->TickSpeed() * 9) == 0)
		{
			GameServer()->PlayerReadyStateBroadcast();
		}

		// checks that only need to happen every second
		// and not every tick
		if(Server()->Tick() % Server()->TickSpeed() == 0)
		{
			CheckForceUnpauseGame();
		}
	}

	if(g_Config.m_SvTournamentChatSmart)
		SmartChatTick();

	// do team-balancing (skip this in survival, done there when a round starts)
	if(IsTeamPlay()) //  && !(m_GameFlags&protocol7::GAMEFLAG_SURVIVAL))
	{
		switch(m_UnbalancedTick)
		{
		case TBALANCE_CHECK:
			CheckTeamBalance();
			break;
		case TBALANCE_OK:
			break;
		default:
			if(g_Config.m_SvTeambalanceTime && Server()->Tick() > m_UnbalancedTick + g_Config.m_SvTeambalanceTime * Server()->TickSpeed() * 60)
				DoTeamBalance();
		}
	}

	// win check
	if((m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED) && !GameServer()->m_World.m_ResetRequested)
	{
		DoWincheckRound();
	}
}

bool CGameControllerBasePvp::OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim)
{
	CGameControllerInstaCore::OnLaserHit(Bounces, From, Weapon, pVictim);
	CPlayer *pPlayer = GameServer()->m_apPlayers[From];
	if(!pPlayer)
		return true;

	if(IsStatTrack() && Bounces != 0)
		pPlayer->m_Stats.m_Wallshots++;

	if(g_Config.m_SvOnlyWallshotKills)
		return Bounces != 0;
	return true;
}

void CGameControllerBasePvp::OnExplosionHits(int OwnerId, CExplosionTarget *pTargets, int NumTargets)
{
	CPlayer *pKiller = GetPlayerOrNullptr(OwnerId);
	if(!pKiller)
		return;

	int HitTeamMates = 0;
	int HitEnemies = 0;
	bool SelfDamage = false;

	for(int i = 0; i < NumTargets; i++)
	{
		CExplosionTarget *pTarget = &pTargets[i];
		int HitId = pTarget->m_pCharacter->GetPlayer()->GetCid();

		// do not count self damage
		// as team or enemy hit
		if(HitId == OwnerId)
		{
			SelfDamage = true;
			continue;
		}

		if(GameServer()->m_pController->IsFriendlyFire(HitId, pKiller->GetCid()))
			HitTeamMates++;
		else
			HitEnemies++;
	}

	// this if statement is a bit bloated
	// but it allows for detailed debug logs
	if(SelfDamage && !HitEnemies)
	{
		// self damage counts as boosting
		// so the hit/misses rate should not be affected
		if(IsStatTrack())
		{
			if(g_Config.m_SvDebugStats)
				log_info("ddnet-insta", "shot did not count because it boosted the shooter");
			pKiller->m_Stats.m_ShotsFired--;
		}
	}
	else if(HitTeamMates && !HitEnemies)
	{
		// boosting mates counts neither as hit nor as miss
		if(IsStatTrack())
		{
			if(g_Config.m_SvDebugStats)
				log_info("ddnet-insta", "shot did not count because it boosted %d team mates", HitTeamMates);
			pKiller->m_Stats.m_ShotsFired--;
		}
	}
}

void CGameControllerBasePvp::OnHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force)
{
	// not sure if these asserts should be kept
	// all of them should be save its just wasting clock cycles
	dbg_assert(pPlayer, "invalid player caused a hammer hit");
	dbg_assert(pTarget, "invalid player received a hammer hit");
	dbg_assert(pTarget->GetCharacter(), "dead player received a hammer hit");

	ApplyFngHammerForce(pPlayer, pTarget, Force);
	FngUnmeltHammerHit(pPlayer, pTarget, Force);
}

void CGameControllerBasePvp::ApplyFngHammerForce(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force)
{
	if(!g_Config.m_SvFngHammer)
		return;

	CCharacter *pTargetChr = pTarget->GetCharacter();
	CCharacter *pFromChr = pPlayer->GetCharacterDeadOrAlive();

	vec2 Dir;
	if(length(pTargetChr->m_Pos - pFromChr->m_Pos) > 0.0f)
		Dir = normalize(pTargetChr->m_Pos - pFromChr->m_Pos);
	else
		Dir = vec2(0.f, -1.f);

	vec2 Push = vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;

	// matches ddnet clients prediction code by default
	// https://github.com/ddnet/ddnet/blob/f9df4a85be4ca94ca91057cd447707bcce16fd94/src/game/client/prediction/entities/character.cpp#L334-L346
	if(GameServer()->m_pController->IsTeamPlay() && pTarget->GetTeam() == pPlayer->GetTeam() && pTargetChr->m_FreezeTime)
	{
		Push.x *= g_Config.m_SvMeltHammerScaleX * 0.01f;
		Push.y *= g_Config.m_SvMeltHammerScaleY * 0.01f;
	}
	else
	{
		Push.x *= g_Config.m_SvHammerScaleX * 0.01f;
		Push.y *= g_Config.m_SvHammerScaleY * 0.01f;
	}

	Force = Push;
}

void CGameControllerBasePvp::FngUnmeltHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force)
{
	CCharacter *pTargetChr = pTarget->GetCharacter();

	// only frozen team mates in fng can be unmelt hammered
	if(!GameServer()->m_pController->IsFngGameType())
		return;
	if(!pTargetChr->m_FreezeTime)
		return;
	if(!GameServer()->m_pController->IsTeamPlay())
		return;
	if(pPlayer->GetTeam() != pTarget->GetTeam())
		return;

	pTargetChr->m_FreezeTime -= Server()->TickSpeed() * 3;

	// make sure we don't got negative and let the ddrace tick trigger the unfreeeze
	if(pTargetChr->m_FreezeTime < 2)
	{
		pTargetChr->m_FreezeTime = 2;

		// reward the unfreezer with one point
		pPlayer->AddScore(1);
		if(GameServer()->m_pController->IsStatTrack())
			pPlayer->m_Stats.m_Unfreezes++;
	}
}

bool CGameControllerBasePvp::IsSpawnProtected(const CPlayer *pVictim, const CPlayer *pKiller) const
{
	// there has to be a valid killer to get spawn protected
	// one should never be spawn protected from the world
	// if the killer left or got invalidated otherwise
	// that should be handled elsewhere
	if(!pKiller)
		return false;
	// if there is no victim nobody needs protection
	if(!pVictim)
		return false;
	if(!pVictim->GetCharacter())
		return false;

	auto &&CheckRecentSpawn = [&](int64_t LastSpawn, int64_t DelayInMs) {
		return (LastSpawn * (int64_t)1000) + (int64_t)Server()->TickSpeed() * DelayInMs > ((int64_t)Server()->Tick() * (int64_t)1000);
	};

	// victim just spawned
	if(CheckRecentSpawn((int64_t)pVictim->GetCharacter()->m_SpawnTick, (int64_t)g_Config.m_SvRespawnProtectionMs))
		return true;

	// killer just spawned
	if(pKiller->GetCharacter())
	{
		if(CheckRecentSpawn((int64_t)pKiller->GetCharacter()->m_SpawnTick, (int64_t)g_Config.m_SvRespawnProtectionMs))
			return true;
	}

	return false;
}

void CGameControllerBasePvp::OnKill(CPlayer *pVictim, CPlayer *pKiller, int Weapon)
{
	if(pKiller->GetCharacter())
	{
		// set attacker's face to happy (taunt!)
		pKiller->GetCharacter()->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
	}
}

bool CGameControllerBasePvp::DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter)
{
	// instagib damage always kills no matter the armor
	// max vanilla weapon damage is katana with 9 dmg
	if(Dmg >= 10)
	{
		pCharacter->SetArmor(0);
		pCharacter->SetHealth(0);
	}

	pCharacter->AddHealth(-Dmg);

	// check for death
	if(pCharacter->Health() <= 0)
	{
		CPlayer *pKiller = GetPlayerOrNullptr(From);
		if(From != pCharacter->GetPlayer()->GetCid() && pKiller)
			OnKill(pCharacter->GetPlayer(), pKiller, Weapon);
		pCharacter->Die(From, Weapon);
		return true;
	}
	return false;
}

bool CGameControllerBasePvp::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	const CPlayer *pPlayer = pCharacter->GetPlayer();
	const CPlayer *pKiller = GetPlayerOrNullptr(From);

	if(From == pPlayer->GetCid())
	{
		if(!m_SelfDamage)
			return true;

		// do not cause self damage with jetpack
		// in any mode ever
		if(Weapon == WEAPON_GUN && pCharacter->Core()->m_Jetpack)
		{
			return true;
		}
	}

	if(pCharacter->m_IsGodmode)
		return true;
	if(From >= 0 && From <= MAX_CLIENTS && GameServer()->m_pController->IsFriendlyFire(pPlayer->GetCid(), From))
		return true;
	if(g_Config.m_SvOnlyHookKills && pKiller)
	{
		const CCharacter *pKillerChr = pKiller->GetCharacter();
		if(pKillerChr)
			if(pKillerChr->HookedPlayer() != pPlayer->GetCid())
				return true;
	}
	if(IsSpawnProtected(pPlayer, pKiller))
		return true;
	return false;
}

void CGameControllerBasePvp::OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	CPlayer *pPlayer = pCharacter->GetPlayer();
	CPlayer *pKiller = GetPlayerOrNullptr(From);

	// only weapons that push the tee around are considered a touch
	// gun and laser do not push (as long as there is no explosive guns/lasers)
	// and shotgun only pushes in ddrace gametypes
	if(Weapon != WEAPON_GUN && Weapon != WEAPON_LASER)
	{
		if(!HasVanillaShotgun(pPlayer) || Weapon != WEAPON_SHOTGUN)
			pPlayer->UpdateLastToucher(From, Weapon);
	}

	if(IsTeamPlay() && pKiller && pPlayer->GetTeam() == pKiller->GetTeam())
	{
		// interaction from team mates protects from spikes in fng
		// and from counting as enemy kill in fly
		pPlayer->UpdateLastToucher(-1, -1);
	}

	if(From == pPlayer->GetCid() && Weapon != WEAPON_LASER)
	{
		// Give back ammo on grenade self push
		// Only if not infinite ammo and activated
		if(Weapon == WEAPON_GRENADE && g_Config.m_SvGrenadeAmmoRegen && g_Config.m_SvGrenadeAmmoRegenSpeedNade)
		{
			pCharacter->SetWeaponAmmo(WEAPON_GRENADE, minimum(pCharacter->GetCore().m_aWeapons[WEAPON_GRENADE].m_Ammo + 1, g_Config.m_SvGrenadeAmmoRegenNum));
		}
	}

	if(Weapon == WEAPON_HAMMER)
	{
		OnHammerHit(pKiller, pPlayer, Force);
	}
}

void CGameControllerBasePvp::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	CPlayer *pPlayer = pCharacter->GetPlayer();
	CPlayer *pKiller = GetPlayerOrNullptr(From);

	if(!pKiller)
		return;

	bool SelfDamage = From == pPlayer->GetCid();
	if(SelfDamage)
		return;

	DoDamageHitSound(From);

	if(Weapon != WEAPON_HAMMER && IsStatTrack())
	{
		pKiller->m_Stats.m_ShotsHit++;
	}

	if(Weapon == WEAPON_GRENADE)
		RefillGrenadesOnHit(pKiller);

	CCharacter *pKillerChar = pKiller->GetCharacter();
	if(g_Config.m_SvReloadTimeOnHit > 0 && Weapon == WEAPON_LASER && pKillerChar)
	{
		float FireDelay;
		int ActiveWeaponTuneIndex = offsetof(CTuningParams, m_HammerFireDelay) / sizeof(CTuneParam) + pKillerChar->m_Core.m_ActiveWeapon;
		pKillerChar->GetTuning(pKillerChar->m_TuneZone)->Get(ActiveWeaponTuneIndex, &FireDelay);
		if(!g_Config.m_SvFastHitFullAuto)
			pKillerChar->m_BlockFullAutoUntilReleaseOrTick = Server()->Tick() + (FireDelay * Server()->TickSpeed() / 1000);
		pKillerChar->m_ReloadTimer = g_Config.m_SvReloadTimeOnHit;
	}

	if(Config()->m_SvFreezeHammer && Weapon == WEAPON_HAMMER)
	{
		CCharacterCore NewCore = pCharacter->GetCore();
		NewCore.m_FreezeEnd = Server()->Tick() + Config()->m_SvFreezeHammer;
		NewCore.m_FreezeStart = Server()->Tick();
		pCharacter->m_FreezeTime = Config()->m_SvFreezeHammer;
		pCharacter->SetCore(NewCore);
	}
}

void CGameControllerBasePvp::RefillGrenadesOnHit(CPlayer *pPlayer)
{
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return;

	// refill nades
	int RefillNades = 0;
	if(g_Config.m_SvGrenadeAmmoRegenOnKill == 1)
		RefillNades = 1;
	else if(g_Config.m_SvGrenadeAmmoRegenOnKill == 2)
		RefillNades = g_Config.m_SvGrenadeAmmoRegenNum;
	if(RefillNades && g_Config.m_SvGrenadeAmmoRegen)
	{
		pChr->SetWeaponAmmo(WEAPON_GRENADE, minimum(pChr->GetCore().m_aWeapons[WEAPON_GRENADE].m_Ammo + RefillNades, g_Config.m_SvGrenadeAmmoRegenNum));
	}
}

bool CGameControllerBasePvp::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	OnAnyDamage(Force, Dmg, From, Weapon, &Character);
	bool ApplyForce = true;
	if(SkipDamage(Dmg, From, Weapon, &Character, ApplyForce))
	{
		Dmg = 0;
		return !ApplyForce;
	}
	OnAppliedDamage(Dmg, From, Weapon, &Character);
	DecreaseHealthAndKill(Dmg, From, Weapon, &Character);
	return false;
}

void CGameControllerBasePvp::AddSpree(class CPlayer *pPlayer)
{
	if(!IsStatTrack())
	{
		pPlayer->m_UntrackedSpree++;
		return;
	}

	pPlayer->m_Spree++;
	const int NumMsg = 5;
	char aBuf[128];

	if(g_Config.m_SvKillingspreeKills > 0 && pPlayer->Spree() % g_Config.m_SvKillingspreeKills == 0)
	{
		static const char aaSpreeMsg[NumMsg][32] = {"is on a killing spree", "is on a rampage", "is dominating", "is unstoppable", "is godlike"};
		int No = pPlayer->Spree() / g_Config.m_SvKillingspreeKills;

		str_format(aBuf, sizeof(aBuf), "'%s' %s with %d kills!", Server()->ClientName(pPlayer->GetCid()), aaSpreeMsg[(No > NumMsg - 1) ? NumMsg - 1 : No], pPlayer->Spree());
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	}
}

void CGameControllerBasePvp::EndSpree(class CPlayer *pPlayer, class CPlayer *pKiller)
{
	if(g_Config.m_SvKillingspreeKills > 0 && pPlayer->Spree() >= g_Config.m_SvKillingspreeKills)
	{
		CCharacter *pChr = pPlayer->GetCharacter();

		if(pChr)
		{
			GameServer()->CreateSound(pChr->m_Pos, SOUND_GRENADE_EXPLODE);
			// GameServer()->CreateExplosion(pChr->m_Pos,  pPlayer->GetCid(), WEAPON_GRENADE, true, -1, -1);
			CNetEvent_Explosion *pEvent = GameServer()->m_Events.Create<CNetEvent_Explosion>(CClientMask());
			if(pEvent)
			{
				pEvent->m_X = (int)pChr->m_Pos.x;
				pEvent->m_Y = (int)pChr->m_Pos.y;
			}

			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "'%s' %d-kills killing spree was ended by '%s'",
				Server()->ClientName(pPlayer->GetCid()), pPlayer->Spree(), Server()->ClientName(pKiller->GetCid()));
			GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		}
	}
	// pPlayer->m_GotAward = false;

	if(pPlayer->m_Stats.m_BestSpree < pPlayer->Spree())
		pPlayer->m_Stats.m_BestSpree = pPlayer->Spree();
	pPlayer->m_Spree = 0;
	pPlayer->m_UntrackedSpree = 0;
}

void CGameControllerBasePvp::OnLoadedNameStats(const CSqlStatsPlayer *pStats, class CPlayer *pPlayer)
{
	if(!pPlayer)
		return;

	pPlayer->m_SavedStats = *pStats;

	if(g_Config.m_SvDebugStats > 1)
	{
		dbg_msg("ddnet-insta", "copied stats:");
		pPlayer->m_SavedStats.Dump(m_pExtraColumns);
	}
}

bool CGameControllerBasePvp::LoadNewPlayerNameData(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return true;

	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return true;

	pPlayer->m_SavedStats.Reset();
	m_pSqlStats->LoadInstaPlayerData(ClientId, m_pStatsTable);

	// consume the event and do not load ddrace times
	return true;
}

bool CGameControllerBasePvp::OnTeamChatCmd(IConsole::IResult *pResult)
{
	if(!g_Config.m_SvTeam)
	{
		return false;
	}

	CPlayer *pPlayer = GameServer()->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return false;

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Only spectators can join ddrace teams");
		return true;
	}

	pPlayer->SetTeam(TEAM_RED, false);
	pPlayer->m_RespawnTick = 0;
	pPlayer->TryRespawn();

	return false;
}

bool CGameControllerBasePvp::OnSetDDRaceTeam(int ClientId, int Team)
{
	// only joining team 0
	// forces players to spectators
	// to avoid players interrupting gameplay
	if(Team != TEAM_FLOCK)
		return false;

	CPlayer *pPlayer = GetPlayerOrNullptr(ClientId);
	if(!pPlayer)
		return false;

	// to avoid recursive loop
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
		return false;

	// to avoid recursive loop
	// because SetTeam kills and killing sets the team
	int OldDDRaceTeam = GameServer()->GetDDRaceTeam(ClientId);
	if(OldDDRaceTeam == TEAM_FLOCK)
		return false;

	if(OldDDRaceTeam == TEAM_SUPER)
		return false;

	// set m_Team directly to avoid recursive loop
	// we do not update the team size because this is not t0
	// and later we again call SetTeam which sends the team change
	// net message so clients are aware of the correct team
	//
	// TODO: revisit this and check if 0.7 works correctly
	//       see https://github.com/ddnet-insta/ddnet-insta/issues/362
	pPlayer->SetTeamRawAndUnsafe(TEAM_SPECTATORS);

	CCharacter *pChr = GameServer()->GetPlayerChar(ClientId);
	if(pChr && pChr->IsAlive())
	{
		// the /team chat command is blocked for in game players
		// so only death should move you to team 0
		// any other cases is unexpected and should be investigated.
		//
		// ideally this branch is never hit because then some assumption is wrong
		// but we might be able to recover using SetTeam so this is not an assert
		//
		// UPDATE: I saw this log in production. Can not reproduce yet.
		//
		// dbg_assert(false,
		// 	"cid=%d changed from ddrace team %d to ddrace team 0 but is still alive",
		// 	ClientId,
		// 	OldDDRaceTeam);
		//
		// UPDATE: Okay I figured it out. This branch can be hit by the rcon command
		//         set_team_ddr and also by kick votes inside of ddrace teams
		//         which use the same rcon command under the hood
		//         if players get moved to t0 with the rcon command they do not die
		//         unless they started the race.

		// in ddracecommands.cpp the character is currently killed with this if statement
		//
		// if((pSelf->GetDDRaceTeam(Target) && pController->Teams().GetDDRaceState(pSelf->m_apPlayers[Target]) == ERaceState::STARTED) || (pChr && pController->Teams().IsPractice(pChr->Team())))
		//
		// if we get out of sync with that in a merge and logic changes I want to know!
		// so we fail loud with an assert

		bool RconCmdShouldHaveKilled =
			(GameServer()->GetDDRaceTeam(ClientId) && Teams().GetDDRaceState(pPlayer) == ERaceState::STARTED) ||
			Teams().IsPractice(pChr->Team());

		// could also assert the bool but i find it hard to read
		if(RconCmdShouldHaveKilled)
		{
			// WARNING: this assert will not actually log when triggered by rcon
			//          because of https://github.com/ddnet/ddnet/issues/11305
			//          so a debugger is needed to catch this
			dbg_assert_failed(
				"cid=%d changed from ddrace team %d to ddrace team 0 but is still alive",
				ClientId,
				OldDDRaceTeam);
		}

		// kill player even if they did not start the race yet
		// to avoid someone bypassing slot limit and entering t0
		pPlayer->KillCharacter(WEAPON_GAME);

		// use respawn delay caused by kill above
		// to move player to spectators
		// because we first want to let the ddnet code run while the player
		// is still in game so there are no unexpected side effects
		// and then after the ddnet code finished we have to move the player
		// to spectators to avoid users bypassing the in game slot limit
		// and enter t0 by escaping a ddrace team
		pPlayer->m_ForceTeam = {
			.m_Tick = pPlayer->m_RespawnTick,
			.m_Team = TEAM_SPECTATORS,
			.m_SpectatorId = SPEC_FREEVIEW};

		// this is expected to only happen when the player gets voted out of the ddrace team
		// or gets moved by an admin with the rcon command set_team_ddr
		SendChatTarget(ClientId, "You were forced to spectators because you left the ddrace team.");
	}
	else
	{
		// this is the expected branch
		// the players death triggered the team change
		// so we have to use the NoKill version of set team
		// otherwise the player is killed twice
		// which causes nullptr issues
		pPlayer->SetTeamNoKill(TEAM_SPECTATORS);
		SendChatTarget(ClientId, "You were forced to spectators because you left the ddrace team. Use /lock to stay in game");
	}

	return false;
}

void CGameControllerBasePvp::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerInstaCore::OnPlayerConnect(pPlayer);
	int ClientId = pPlayer->GetCid();
	pPlayer->ResetStats();

	// init the player
	Score()->PlayerData(ClientId)->Reset();

	if(!LoadNewPlayerNameData(ClientId))
	{
		Score()->LoadPlayerData(ClientId);
	}

	if(!Server()->ClientPrevIngame(ClientId))
	{
		GameServer()->AlertOnSpecialInstagibConfigs(ClientId);
		GameServer()->ShowCurrentInstagibConfigsMotd(ClientId);
	}

	CheckReadyStates();
	SendGameInfo(ClientId); // update game info
}

void CGameControllerBasePvp::OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason)
{
	CGameControllerInstaCore::OnPlayerDisconnect(pPlayer, pReason);

	if(GameState() != IGS_END_ROUND)
		SaveStatsOnDisconnect(pPlayer);

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		if(GameServer()->GetDDRaceTeam(pPlayer->GetCid()) == 0)
			--m_aTeamSize[pPlayer->GetTeam()];
		m_UnbalancedTick = TBALANCE_CHECK;
	}

	CheckReadyStates(pPlayer->GetCid());
}

// TODO: can we move this to the core controller?
void CGameControllerBasePvp::DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	if(!IsValidTeam(Team))
		return;

	// has to be saved for later
	// because the set team operation kills the character
	// and then we lose the team information
	int DDRaceTeam = GameServer()->GetDDRaceTeam(pPlayer->GetCid());

	if(Team == pPlayer->GetTeam())
		return;

	int OldTeam = pPlayer->GetTeam(); // ddnet-insta
	pPlayer->SetTeamSpoofed(Team);
	int ClientId = pPlayer->GetCid();

	char aBuf[128];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(ClientId), GameServer()->m_pController->GetTeamName(Team));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf, CGameContext::FLAG_SIX);
	}

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", ClientId, Server()->ClientName(ClientId), Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// OnPlayerInfoChange(pPlayer);

	// ddnet-insta

	// https://github.com/ddnet-insta/ddnet-insta/issues/388
	// makes sure changing team during a fng sacrifice does not give the
	// player a wrong shire punishment
	//
	// also invalidates block or fly kills when the killer joined spectators between starting the kill (hooking, shooting)
	// and the actual death of the victim
	pPlayer->ResetOwnLastTouchOnAllOtherPlayers();

	if(OldTeam == TEAM_SPECTATORS)
	{
		GameServer()->AlertOnSpecialInstagibConfigs(pPlayer->GetCid());
		GameServer()->ShowCurrentInstagibConfigsMotd(pPlayer->GetCid());
	}

	// update effected game settings
	if(OldTeam != TEAM_SPECTATORS)
	{
		if(DDRaceTeam == 0)
			--m_aTeamSize[OldTeam];
		m_UnbalancedTick = TBALANCE_CHECK;
	}
	if(Team != TEAM_SPECTATORS)
	{
		if(DDRaceTeam == 0)
			++m_aTeamSize[Team];
		m_UnbalancedTick = TBALANCE_CHECK;
		// if(m_GameState == IGS_WARMUP_GAME && HasEnoughPlayers())
		// 	SetGameState(IGS_WARMUP_GAME, 0);
		// pPlayer->m_IsReadyToPlay = !IsPlayerReadyMode();
		// if(m_GameFlags&GAMEFLAG_SURVIVAL)
		// 	pPlayer->m_RespawnDisabled = GetStartRespawnState();
	}
	CheckReadyStates();
}

bool CGameControllerBasePvp::BlockFirstShotOnSpawn(class CCharacter *pChr, int Weapon) const
{
	// WEAPON_GUN is not full auto
	// this makes sure that vanilla gamemodes are not affected
	// by any side effects that this fix might have
	if(Weapon == WEAPON_GUN)
		return false;

	// if a player holds down the fire key forever
	// we eventually activate the full auto weapon
	int TicksAlive = Server()->Tick() - pChr->m_SpawnTick;
	constexpr int HalfSecond = SERVER_TICK_SPEED / 2;
	if(TicksAlive > HalfSecond)
		return false;

	// all the ddrace edge cases still apply
	// except the ones that activate full auto for certain weapons (UPDATE: why not those???)
	bool FullAuto = false;
	if(pChr->m_Core.m_Jetpack && pChr->m_Core.m_ActiveWeapon == WEAPON_GUN)
		FullAuto = true;
	// allow firing directly after coming out of freeze or being unfrozen
	// by something
	if(pChr->m_FrozenLastTick)
		FullAuto = true;

	// check if we gonna fire
	if(CountInput(pChr->m_LatestPrevInput.m_Fire, pChr->m_LatestInput.m_Fire).m_Presses)
		return false;
	if(FullAuto && (pChr->m_LatestInput.m_Fire & 1) && pChr->m_Core.m_aWeapons[pChr->m_Core.m_ActiveWeapon].m_Ammo)
		return false;
	return true;
}

bool CGameControllerBasePvp::BlockFullAutoUntilRepress(class CCharacter *pChr, int Weapon) const
{
	// TODO: this might also apply to other auto weapons
	//       but i dont want to fully test all of them now
	//       so this will be laser only for now making sure no new bugs are introduced
	//       https://github.com/ddnet-insta/ddnet-insta/issues/383
	if(Weapon != WEAPON_LASER)
		return false;
	if(Server()->Tick() > pChr->m_BlockFullAutoUntilReleaseOrTick)
		pChr->m_BlockFullAutoUntilReleaseOrTick = 0;
	if(pChr->m_BlockFullAutoUntilReleaseOrTick == 0)
		return false;

	bool FullAuto = false;
	if(pChr->m_Core.m_ActiveWeapon == WEAPON_GRENADE || pChr->m_Core.m_ActiveWeapon == WEAPON_SHOTGUN || pChr->m_Core.m_ActiveWeapon == WEAPON_LASER)
		FullAuto = true;
	if(pChr->m_Core.m_Jetpack && pChr->m_Core.m_ActiveWeapon == WEAPON_GUN)
		FullAuto = true;
	// allow firing directly after coming out of freeze or being unfrozen
	// by something
	if(pChr->m_FrozenLastTick)
		FullAuto = true;

	// check if we gonna fire
	if(CountInput(pChr->m_LatestPrevInput.m_Fire, pChr->m_LatestInput.m_Fire).m_Presses)
	{
		// fresh press unlocks full auto if it was locked
		pChr->m_BlockFullAutoUntilReleaseOrTick = 0;
		return false;
	}
	if(FullAuto && (pChr->m_LatestInput.m_Fire & 1) && pChr->m_Core.m_aWeapons[pChr->m_Core.m_ActiveWeapon].m_Ammo)
	{
		if(pChr->m_BlockFullAutoUntilReleaseOrTick != 0)
			return true; // block! this is what the entire method is really about
	}
	return false;
}

bool CGameControllerBasePvp::OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos)
{
	// https://github.com/ddnet-insta/ddnet-insta/issues/289
	// left clicking during death screen can decrease the spawn delay
	// but in modes where players spawn with full auto weapons such as
	// grenade and laser this can fire a shot on spawn
	//
	// so this shot is intentionally blocked here
	// to avoid messing with hit accuracy stats
	// and also fix players doing potentially unwanted kills
	// by just trying to respawn
	if(BlockFirstShotOnSpawn(&Character, Weapon))
		return true;
	// https://github.com/ddnet-insta/ddnet-insta/issues/375
	// sv_reload_time_on_hit can make weapons shoot faster on hit
	// this was made for laser fng to be able to fight multiple enemies
	// but because the laser is a auto weapon (meaning holding fire keeps shooting)
	// it can cause accidental shots.
	// So in that case the laser will be turned into a non auto weapon until
	// the player repressed the fire button.
	if(BlockFullAutoUntilRepress(&Character, Weapon))
		return true;

	if(IsStatTrack() && Weapon != WEAPON_HAMMER)
		Character.GetPlayer()->m_Stats.m_ShotsFired++;

	if(g_Config.m_SvGrenadeAmmoRegenResetOnFire && Character.m_Core.m_ActiveWeapon == WEAPON_GRENADE)
		Character.m_Core.m_aWeapons[Character.m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
	if(Character.m_Core.m_aWeapons[Character.m_Core.m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		Character.m_Core.m_aWeapons[Character.m_Core.m_ActiveWeapon].m_Ammo--;

	if(Weapon == WEAPON_GUN)
	{
		if(!Character.m_Core.m_Jetpack || !Character.m_pPlayer->m_NinjaJetpack || Character.m_Core.m_HasTelegunGun)
		{
			int Lifetime = (int)(Server()->TickSpeed() * Character.GetTuning(Character.m_TuneZone)->m_GunLifetime);

			new CVanillaProjectile(
				Character.GameWorld(),
				WEAPON_GUN, //Type
				Character.m_pPlayer->GetCid(), //Owner
				ProjStartPos, //Pos
				Direction, //Dir
				Lifetime, //Span
				false, //Freeze
				false, //Explosive
				-1, //SoundImpact
				MouseTarget //InitDir
			);

			GameServer()->CreateSound(Character.m_Pos, SOUND_GUN_FIRE, Character.TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
		}
	}
	else if(Weapon == WEAPON_SHOTGUN)
	{
		int ShotSpread = 2;

		for(int i = -ShotSpread; i <= ShotSpread; ++i) // NOLINT(clang-analyzer-unix.Malloc)
		{
			float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
			float Angle = angle(Direction);
			Angle += Spreading[i + 2];
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)GameServer()->GlobalTuning()->m_ShotgunSpeeddiff, 1.0f, v);

			// TODO: not sure about Dir and InitDir and prediction

			new CVanillaProjectile(
				Character.GameWorld(),
				WEAPON_SHOTGUN, // Type
				Character.GetPlayer()->GetCid(), // Owner
				ProjStartPos, // Pos
				direction(Angle) * Speed, // Dir
				(int)(Server()->TickSpeed() * GameServer()->GlobalTuning()->m_ShotgunLifetime), // Span
				false, // Freeze
				false, // Explosive
				-1, // SoundImpact
				vec2(cosf(Angle), sinf(Angle)) * Speed); // InitDir
		}

		GameServer()->CreateSound(Character.m_Pos, SOUND_SHOTGUN_FIRE); // NOLINT(clang-analyzer-unix.Malloc)
	}
	else
	{
		return false;
	}

	Character.m_AttackTick = Server()->Tick();

	if(!Character.m_ReloadTimer)
	{
		float FireDelay;
		Character.GetTuning(Character.m_TuneZone)->Get(offsetof(CTuningParams, m_HammerFireDelay) / sizeof(CTuneParam) + Character.m_Core.m_ActiveWeapon, &FireDelay);
		Character.m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
	}

	return true;
}

int CGameControllerBasePvp::NumActivePlayers()
{
	int Active = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && (pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_IsDead))
			Active++;
	return Active;
}

int CGameControllerBasePvp::NumAlivePlayers()
{
	int Alive = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetCharacter())
			Alive++;
	return Alive;
}

int CGameControllerBasePvp::NumNonDeadActivePlayers()
{
	int Alive = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && !pPlayer->m_IsDead && pPlayer->GetTeam() != TEAM_SPECTATORS)
			Alive++;
	return Alive;
}

int CGameControllerBasePvp::GetHighestSpreeClientId()
{
	int ClientId = -1;
	int Spree = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->Spree() <= Spree)
			continue;

		ClientId = pPlayer->GetCid();
		Spree = pPlayer->Spree();
	}
	return ClientId;
}

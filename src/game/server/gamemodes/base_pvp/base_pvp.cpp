#include <base/log.h>
#include <base/system.h>
#include <cstdint>
#include <engine/server/server.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/generated/server_data.h>
#include <game/race_state.h>
#include <game/server/entities/character.h>
#include <game/server/entities/ddnet_pvp/pvp_laser.h>
#include <game/server/entities/ddnet_pvp/pvp_projectile.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/enums.h>
#include <game/server/instagib/laser_text.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/structs.h>
#include <game/server/instagib/version.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/server/teams.h>
#include <game/version.h>

#include <game/server/instagib/antibob.h>

#include "base_pvp.h"

CGameControllerPvp::CGameControllerPvp(class CGameContext *pGameServer) :
	CGameControllerDDRace(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;

	UpdateSpawnWeapons(true, true);

	m_AllowSkinColorChange = true;

	GameServer()->Tuning()->Set("gun_curvature", 1.25f);
	GameServer()->Tuning()->Set("gun_speed", 2200);
	GameServer()->Tuning()->Set("shotgun_curvature", 1.25f);
	GameServer()->Tuning()->Set("shotgun_speed", 2750);
	GameServer()->Tuning()->Set("shotgun_speeddiff", 0.8f);

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
	// this contructor is not called on "restart" commands
	if(g_Config.m_SvTournamentChatSmart)
		g_Config.m_SvTournamentChat = 0;

	m_vFrozenQuitters.clear();

	m_UnbalancedTick = TBALANCE_OK;

	g_AntibobContext.m_pConsole = Console();
}

void CGameControllerPvp::OnReset()
{
	CGameControllerDDRace::OnReset();

	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_IsReadyToPlay = true;
		pPlayer->m_ScoreStartTick = Server()->Tick();
	}
}

void CGameControllerPvp::OnInit()
{
	if(GameFlags() & GAMEFLAG_FLAGS)
	{
		m_pSqlStats->CreateFastcapTable();
	}
}

void CGameControllerPvp::OnRoundStart()
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

void CGameControllerPvp::OnRoundEnd()
{
	dbg_msg("ddnet-insta", "match end");

	if(g_Config.m_SvTournamentChatSmart)
	{
		if(g_Config.m_SvTournamentChat)
			GameServer()->SendChat(-1, TEAM_ALL, "Spectators can use public chat again");
		g_Config.m_SvTournamentChat = 0;
	}

	PublishRoundEndStats();

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

CGameControllerPvp::~CGameControllerPvp()
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

// called on round init and on join
void CGameControllerPvp::RoundInitPlayer(CPlayer *pPlayer)
{
	pPlayer->m_IsDead = false;
	pPlayer->m_KillerId = -1;
}

// this is only called once on connect
// NOT ON ROUND END
void CGameControllerPvp::InitPlayer(CPlayer *pPlayer)
{
	pPlayer->m_Spree = 0;
	pPlayer->m_UntrackedSpree = 0;
	pPlayer->ResetStats();
	pPlayer->m_SavedStats.Reset();

	pPlayer->m_IsReadyToPlay = !GameServer()->m_pController->IsPlayerReadyMode();
	pPlayer->m_DeadSpecMode = false;
	pPlayer->m_GameStateBroadcast = false;
	pPlayer->m_Score = 0; // ddnet-insta
	pPlayer->m_DisplayScore = GameServer()->m_DisplayScore;
	pPlayer->m_JoinTime = time_get();

	RoundInitPlayer(pPlayer);
}

int CGameControllerPvp::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
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

int CGameControllerPvp::SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags)
{
	return GAMEINFOFLAG2_HUD_AMMO | GAMEINFOFLAG2_HUD_HEALTH_ARMOR;
}

int CGameControllerPvp::SnapPlayerFlags7(int SnappingClient, CPlayer *pPlayer, int PlayerFlags7)
{
	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return PlayerFlags7;

	if(pPlayer->m_IsDead && (!pPlayer->GetCharacter() || !pPlayer->GetCharacter()->IsAlive()))
		PlayerFlags7 |= protocol7::PLAYERFLAG_DEAD;
	// hack to let 0.7 players vote as spectators
	if(g_Config.m_SvSpectatorVotes && g_Config.m_SvSpectatorVotesSixup && pPlayer->GetTeam() == TEAM_SPECTATORS)
		PlayerFlags7 |= protocol7::PLAYERFLAG_DEAD;
	if(g_Config.m_SvHideAdmins && Server()->GetAuthedState(SnappingClient) == AUTHED_NO)
		PlayerFlags7 &= ~(protocol7::PLAYERFLAG_ADMIN);
	return PlayerFlags7;
}

void CGameControllerPvp::SnapPlayer6(int SnappingClient, CPlayer *pPlayer, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo)
{
	if(!IsGameRunning() &&
		GameServer()->m_World.m_Paused &&
		GameState() != IGameController::IGS_END_ROUND &&
		pPlayer->GetTeam() != TEAM_SPECTATORS &&
		(!IsPlayerReadyMode() || pPlayer->m_IsReadyToPlay))
	{
		char aReady[512];
		char aName[64];
		static const int MaxNameLen = MAX_NAME_LENGTH - (str_length("\xE2\x9C\x93") + 2);
		str_truncate(aName, sizeof(aName), Server()->ClientName(pPlayer->GetCid()), MaxNameLen);
		str_format(aReady, sizeof(aReady), "\xE2\x9C\x93 %s", aName);
		// 0.7 puts the checkmark at the end
		// we put it in the beginning because ddnet scoreboard cuts off long names
		// such as WWWWWWWWWW... which would also hide the checkmark in the end
		StrToInts(&pClientInfo->m_Name0, 4, aReady);
	}
}

void CGameControllerPvp::SnapDDNetPlayer(int SnappingClient, CPlayer *pPlayer, CNetObj_DDNetPlayer *pDDNetPlayer)
{
	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return;

	if(g_Config.m_SvHideAdmins && Server()->GetAuthedState(SnappingClient) == AUTHED_NO)
		pDDNetPlayer->m_AuthLevel = AUTHED_NO;
}

int CGameControllerPvp::SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore)
{
	CPlayer *pSnapReceiver = GetPlayerOrNullptr(SnappingClient);
	if(!pSnapReceiver)
		return DDRaceScore;

	int Score = pPlayer->m_Score.value_or(0);

	// alawys force display round score if the game ended
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

bool CGameControllerPvp::IsGrenadeGameType() const
{
	// TODO: this should be done with some cleaner spawnweapons/available weapons enum flag thing
	if(IsZcatchGameType())
	{
		return m_SpawnWeapons == SPAWN_WEAPON_GRENADE;
	}
	return IsVanillaGameType() || m_pGameType[0] == 'g';
}

void CGameControllerPvp::OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks)
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

bool CGameControllerPvp::ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos)
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

bool CGameControllerPvp::ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos)
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

bool CGameControllerPvp::OnClientPacket(int ClientId, bool Sys, int MsgId, CNetChunk *pPacket, CUnpacker *pUnpacker)
{
	// make a copy so we can consume fields
	// without breaking the state for the server
	// in case we pass the packet on
	CUnpacker Unpacker = *pUnpacker;
	bool Vital = pPacket->m_Flags & NET_CHUNKFLAG_VITAL;

	if(Sys && MsgId == NETMSG_RCON_AUTH && Vital && Server()->IsSixup(ClientId))
	{
		const char *pCredentials = Unpacker.GetString(CUnpacker::SANITIZE_CC);
		if(Unpacker.Error())
			return false;

		// check if 0.7 player sends valid credentials for
		// a ddnet rcon account in the format username:pass
		// in that case login and drop the message
		if(Server()->SixupUsernameAuth(ClientId, pCredentials))
			return true;
	}

	return false;
}

void CGameControllerPvp::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
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

void CGameControllerPvp::OnShowRank(int Rank, int RankedScore, const char *pRankType, class CPlayer *pRequestingPlayer, const char *pRequestedName)
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

void CGameControllerPvp::OnUpdateSpectatorVotesConfig()
{
	// spec votes was activated
	// spoof all specatators to in game dead specs for 0.7
	// so the client side knows it can call votes
	if(g_Config.m_SvSpectatorVotes && g_Config.m_SvSpectatorVotesSixup)
	{
		for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		{
			if(!pPlayer)
				continue;
			if(pPlayer->GetTeam() != TEAM_SPECTATORS)
				continue;
			if(!Server()->IsSixup(pPlayer->GetCid()))
				continue;

			// Every sixup client only needs to see it self as spectator
			// It does not care about others
			protocol7::CNetMsg_Sv_Team Msg;
			Msg.m_ClientId = pPlayer->GetCid();
			Msg.m_Team = TEAM_RED; // fake
			Msg.m_Silent = true;
			Msg.m_CooldownTick = 0;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, pPlayer->GetCid());

			pPlayer->m_IsFakeDeadSpec = true;
		}
	}
	else
	{
		// spec votes were deactivated
		// so revert spoofed in game teams back to regular spectators
		// make sure this does not mess with ACTUAL dead spec tees
		for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		{
			if(!pPlayer)
				continue;
			if(!Server()->IsSixup(pPlayer->GetCid()))
				continue;
			if(!pPlayer->m_IsFakeDeadSpec)
				continue;

			if(pPlayer->GetTeam() != TEAM_SPECTATORS)
			{
				dbg_msg("ddnet-insta", "ERROR: tried to move player back to team=%d but expected spectators", pPlayer->GetTeam());
			}

			protocol7::CNetMsg_Sv_Team Msg;
			Msg.m_ClientId = pPlayer->GetCid();
			Msg.m_Team = pPlayer->GetTeam(); // restore real team
			Msg.m_Silent = true;
			Msg.m_CooldownTick = 0;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, pPlayer->GetCid());

			pPlayer->m_IsFakeDeadSpec = false;
		}
	}
}

bool CGameControllerPvp::IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage)
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

bool CGameControllerPvp::IsLoser(const CPlayer *pPlayer)
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

bool CGameControllerPvp::IsStatTrack(char *pReason, int SizeOfReason)
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

void CGameControllerPvp::SaveStatsOnRoundEnd(CPlayer *pPlayer)
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

void CGameControllerPvp::SaveStatsOnDisconnect(CPlayer *pPlayer)
{
	if(!pPlayer)
		return;
	if(!pPlayer->m_Stats.HasValues())
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

bool CGameControllerPvp::OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];

	if(pPlayer->GetTeam() == TEAM_SPECTATORS && !g_Config.m_SvSpectatorVotes)
	{
		// SendChatTarget(ClientId, "Spectators aren't allowed to vote.");
		return true;
	}
	return false;
}

// called before spam protection on client team join request
// return true to consume the event and not run the base controller code
bool CGameControllerPvp::OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;

	if(GameServer()->m_World.m_Paused)
	{
		if(!g_Config.m_SvAllowTeamChangeDuringPause)
		{
			GameServer()->SendChatTarget(pPlayer->GetCid(), "Changing teams while the game is paused is currently disabled.");
			return true;
		}
	}

	int Team = pMsg->m_Team;

	// user joins the spectators while allow spec is on
	// we have to mark him as fake dead spec
	if(Server()->IsSixup(ClientId) && g_Config.m_SvSpectatorVotes && g_Config.m_SvSpectatorVotesSixup && !pPlayer->m_IsFakeDeadSpec)
	{
		if(Team == TEAM_SPECTATORS)
		{
			pPlayer->m_IsFakeDeadSpec = true;
			return false;
		}
	}

	if(Server()->IsSixup(ClientId) && g_Config.m_SvSpectatorVotes && pPlayer->m_IsFakeDeadSpec)
	{
		if(Team != TEAM_SPECTATORS)
		{
			// This should be in all cases coming from the hacked recursion branch below
			//
			// the 0.7 client should think it is in game
			// so it should never display a join game button
			// only a join spectators button
			return false;
		}

		pPlayer->m_IsFakeDeadSpec = false;

		// hijack and drop
		// and then call it again
		// as a hack to edit the team
		CNetMsg_Cl_SetTeam Msg;
		Msg.m_Team = TEAM_RED;
		GameServer()->OnSetTeamNetMessage(&Msg, ClientId);
		return true;
	}
	return false;
}

int CGameControllerPvp::GetPlayerTeam(class CPlayer *pPlayer, bool Sixup)
{
	if(g_Config.m_SvTournament)
		return IGameController::GetPlayerTeam(pPlayer, Sixup);

	// hack to let 0.7 players vote as spectators
	if(g_Config.m_SvSpectatorVotes && g_Config.m_SvSpectatorVotesSixup && Sixup && pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		return TEAM_RED;
	}

	return IGameController::GetPlayerTeam(pPlayer, Sixup);
}

bool CGameControllerPvp::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	const CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
	if(pPlayer && pPlayer->IsPaused())
	{
		if(pErrorReason)
			str_copy(pErrorReason, "Use /pause first then you can kill", ErrorReasonSize);
		return false;
	}
	if(Team == TEAM_SPECTATORS || (pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS))
		return true;

	if(FreeInGameSlots())
		return true;

	if(pErrorReason)
		str_format(pErrorReason, ErrorReasonSize, "Only %d active players are allowed", Server()->MaxClients() - g_Config.m_SvSpectatorSlots);
	return false;
}

int CGameControllerPvp::GetAutoTeam(int NotThisId)
{
	if(Config()->m_SvTournamentMode)
		return TEAM_SPECTATORS;

	// determine new team
	int Team = TEAM_RED;
	if(IsTeamPlay())
	{
#ifdef CONF_DEBUG
		if(!Config()->m_DbgStress) // this will force the auto balancer to work overtime aswell
#endif
			Team = m_aTeamSize[TEAM_RED] > m_aTeamSize[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;
	}

	// check if there're enough player slots left
	if(FreeInGameSlots())
	{
		if(GameServer()->GetDDRaceTeam(NotThisId) == 0)
			++m_aTeamSize[Team];
		return Team;
	}
	return TEAM_SPECTATORS;
}

void CGameControllerPvp::SendChatTarget(int To, const char *pText, int Flags) const
{
	GameServer()->SendChatTarget(To, pText, Flags);
}

void CGameControllerPvp::SendChat(int ClientId, int Team, const char *pText, int SpamProtectionClientId, int Flags)
{
	GameServer()->SendChat(ClientId, Team, pText, SpamProtectionClientId, Flags);
}

void CGameControllerPvp::UpdateSpawnWeapons(bool Silent, bool Apply)
{
	// these gametypes are weapon bound
	// so the always overwrite sv_spawn_weapons
	if(m_pGameType[0] == 'g' // gDM, gCTF
		|| m_pGameType[0] == 'i' // iDM, iCTF
		|| m_pGameType[0] == 'C' // CTF*
		|| m_pGameType[0] == 'T' // TDM*
		|| m_pGameType[0] == 'D') // DM*
	{
		if(!Silent)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "WARNING: sv_spawn_weapons only has an effect in zCatch");
		}
	}
	if(str_find_nocase(m_pGameType, "fng"))
	{
		if(!Silent)
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "WARNING: use sv_gametype fng/solofng/bolofng/boomfng to change weapons in fng");
		return;
	}

	if(Apply)
	{
		const char *pWeapons = Config()->m_SvSpawnWeapons;
		if(!str_comp_nocase(pWeapons, "grenade"))
			m_SpawnWeapons = SPAWN_WEAPON_GRENADE;
		else if(!str_comp_nocase(pWeapons, "laser") || !str_comp_nocase(pWeapons, "rifle"))
			m_SpawnWeapons = SPAWN_WEAPON_LASER;
		else
		{
			dbg_msg("ddnet-insta", "WARNING: invalid spawn weapon falling back to grenade");
			m_SpawnWeapons = SPAWN_WEAPON_GRENADE;
		}

		m_DefaultWeapon = GetDefaultWeaponBasedOnSpawnWeapons();
	}
	else if(!Silent)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "WARNING: reload required for spawn weapons to apply");
	}
}

void CGameControllerPvp::ModifyWeapons(IConsole::IResult *pResult, void *pUserData,
	int Weapon, bool Remove)
{
	CGameControllerPvp *pSelf = (CGameControllerPvp *)pUserData;
	CCharacter *pChr = GameServer()->GetPlayerChar(pResult->m_ClientId);
	if(!pChr)
		return;

	if(std::clamp(Weapon, -1, NUM_WEAPONS - 1) != Weapon)
	{
		pSelf->GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"invalid weapon id");
		return;
	}

	if(Weapon == -1)
	{
		pChr->GiveWeapon(WEAPON_SHOTGUN, Remove);
		pChr->GiveWeapon(WEAPON_GRENADE, Remove);
		pChr->GiveWeapon(WEAPON_LASER, Remove);
	}
	else
	{
		pChr->GiveWeapon(Weapon, Remove);
	}

	pChr->m_DDRaceState = ERaceState::CHEATED;
}

int CGameControllerPvp::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	CGameControllerDDRace::OnCharacterDeath(pVictim, pKiller, Weapon);

	if(pVictim->HasRainbow())
		pVictim->Rainbow(false);

	// this is the vanilla base default respawn delay
	// it can not be configured
	// but it will overwritten by configurable delays in almost all cases
	// so this only a fallback
	int DelayInMs = 500;

	if(Weapon == WEAPON_SELF)
		DelayInMs = g_Config.m_SvSelfKillRespawnDelayMs;
	else if(Weapon == WEAPON_WORLD)
		DelayInMs = g_Config.m_SvWorldKillRespawnDelayMs;
	else if(Weapon == WEAPON_GAME)
		DelayInMs = g_Config.m_SvGameKillRespawnDelayMs;
	else if(pKiller && pVictim->GetPlayer() != pKiller)
		DelayInMs = g_Config.m_SvEnemyKillRespawnDelayMs;
	else if(pKiller && pVictim->GetPlayer() == pKiller)
		DelayInMs = g_Config.m_SvSelfDamageRespawnDelayMs;

	int DelayInTicks = (int)(Server()->TickSpeed() * ((float)DelayInMs / 1000.0f));
	pVictim->GetPlayer()->m_RespawnTick = Server()->Tick() + DelayInTicks;

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
			if(g_Config.m_SvReloadTimeOnHit > 0 && Weapon == WEAPON_LASER && !IsFngGameType())
			{
				pKiller->GetCharacter()->m_ReloadTimer = g_Config.m_SvReloadTimeOnHit;
			}
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

void CGameControllerPvp::CheckForceUnpauseGame()
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

void CGameControllerPvp::Tick()
{
	CGameControllerDDRace::Tick();

	if(m_TicksUntilShutdown)
	{
		m_TicksUntilShutdown--;
		if(m_TicksUntilShutdown < 1)
		{
			Server()->ShutdownServer();
		}
	}

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

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		OnPlayerTick(pPlayer);

		if(!pPlayer->GetCharacter())
			continue;

		OnCharacterTick(pPlayer->GetCharacter());
	}

	if(g_Config.m_SvAnticamper && !GameServer()->m_World.m_Paused)
		Anticamper();
	if(g_Config.m_SvTournamentChatSmart)
		SmartChatTick();

	if(m_ReleaseAllFrozenQuittersTick < Server()->Tick() && !m_vFrozenQuitters.empty())
	{
		log_info("ddnet-insta", "all freeze quitter punishments expired. cleaning up ...");
		m_vFrozenQuitters.clear();
	}

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

void CGameControllerPvp::OnPlayerTick(class CPlayer *pPlayer)
{
	pPlayer->InstagibTick();

	if(GameServer()->m_World.m_Paused)
	{
		// this is needed for the smart tournament chat
		// otherwise players get marked as afk during pause
		// and then the game is considered not competitive anymore
		// which is wrong
		pPlayer->UpdatePlaytime();

		// all these are set in player.cpp
		// ++m_RespawnTick;
		// ++m_DieTick;
		// ++m_PreviousDieTick;
		// ++m_JoinTick;
		// ++m_LastActionTick;
		// ++m_TeamChangeTick;
		++pPlayer->m_ScoreStartTick;
	}

	if(pPlayer->m_GameStateBroadcast)
	{
		char aBuf[512];
		str_format(
			aBuf,
			sizeof(aBuf),
			"GameState: %s                                                                                                                               ",
			GameStateToStr(GameState()));
		GameServer()->SendBroadcast(aBuf, pPlayer->GetCid());
	}

	// last toucher for fng and block
	CCharacter *pChr = pPlayer->GetCharacter();
	if(pChr && pChr->IsAlive())
	{
		int HookedId = pChr->Core()->HookedPlayer();
		if(HookedId >= 0 && HookedId < MAX_CLIENTS)
		{
			CPlayer *pHooked = GameServer()->m_apPlayers[HookedId];
			if(pHooked)
			{
				pHooked->UpdateLastToucher(pChr->GetPlayer()->GetCid());
			}
		}
	}
}

void CGameControllerPvp::OnCharacterTick(CCharacter *pChr)
{
	if(pChr->GetPlayer()->m_PlayerFlags & PLAYERFLAG_CHATTING)
		pChr->GetPlayer()->m_TicksSpentChatting++;
}

bool CGameControllerPvp::OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[From];
	if(!pPlayer)
		return true;

	if(IsStatTrack() && Bounces != 0)
		pPlayer->m_Stats.m_Wallshots++;

	if(!IsFngGameType())
		pVictim->UnFreeze();

	if(g_Config.m_SvOnlyWallshotKills)
		return Bounces != 0;
	return true;
}

void CGameControllerPvp::OnExplosionHits(int OwnerId, CExplosionTarget *pTargets, int NumTargets)
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

void CGameControllerPvp::OnHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force)
{
	// not sure if these asserts should be kept
	// all of them should be save its just wasting clock cycles
	dbg_assert(pPlayer, "invalid player caused a hammer hit");
	dbg_assert(pTarget, "invalid player received a hammer hit");
	dbg_assert(pTarget->GetCharacter(), "dead player received a hammer hit");

	ApplyFngHammerForce(pPlayer, pTarget, Force);
	FngUnmeltHammerHit(pPlayer, pTarget, Force);
}

void CGameControllerPvp::ApplyFngHammerForce(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force)
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

void CGameControllerPvp::FngUnmeltHammerHit(CPlayer *pPlayer, CPlayer *pTarget, vec2 &Force)
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

bool CGameControllerPvp::IsSpawnProtected(const CPlayer *pVictim, const CPlayer *pKiller) const
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

void CGameControllerPvp::ApplyVanillaDamage(int &Dmg, int From, int Weapon, CCharacter *pCharacter)
{
	CPlayer *pPlayer = pCharacter->GetPlayer();
	if(From == pPlayer->GetCid())
	{
		// m_pPlayer only inflicts half damage on self
		Dmg = maximum(1, Dmg / 2);
	}

	pCharacter->m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < pCharacter->m_DamageTakenTick + 25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(pCharacter->m_Pos, pCharacter->m_DamageTaken * 0.25f, Dmg);
	}
	else
	{
		pCharacter->m_DamageTaken = 0;
		GameServer()->CreateDamageInd(pCharacter->m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(pCharacter->m_Armor)
		{
			if(Dmg > 1)
			{
				pCharacter->m_Health--;
				Dmg--;
			}

			if(Dmg > pCharacter->m_Armor)
			{
				Dmg -= pCharacter->m_Armor;
				pCharacter->m_Armor = 0;
			}
			else
			{
				pCharacter->m_Armor -= Dmg;
				Dmg = 0;
			}
		}
	}

	pCharacter->m_DamageTakenTick = Server()->Tick();

	if(Dmg > 2)
		GameServer()->CreateSound(pCharacter->m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(pCharacter->m_Pos, SOUND_PLAYER_PAIN_SHORT);
}

void CGameControllerPvp::OnKill(CPlayer *pVictim, CPlayer *pKiller, int Weapon)
{
	if(pKiller->GetCharacter())
	{
		// set attacker's face to happy (taunt!)
		pKiller->GetCharacter()->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
	}
}

bool CGameControllerPvp::DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter)
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

bool CGameControllerPvp::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
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

void CGameControllerPvp::OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	CPlayer *pPlayer = pCharacter->GetPlayer();
	CPlayer *pKiller = GetPlayerOrNullptr(From);

	// only weapons that push the tee around are considerd a touch
	// gun and laser do not push (as long as there is no explosive guns/lasers)
	// and shotgun only pushes in ddrace gametypes
	if(Weapon != WEAPON_GUN && Weapon != WEAPON_LASER)
	{
		if(!HasVanillaShotgun(pPlayer) || Weapon != WEAPON_SHOTGUN)
			pPlayer->UpdateLastToucher(From);
	}

	if(IsTeamPlay() && pKiller && pPlayer->GetTeam() == pKiller->GetTeam())
	{
		// interaction from team mates protects from spikes in fng
		// and from counting as enemy kill in fly
		pPlayer->UpdateLastToucher(-1);
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

void CGameControllerPvp::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
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
}

void CGameControllerPvp::RefillGrenadesOnHit(CPlayer *pPlayer)
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

bool CGameControllerPvp::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
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

void CGameControllerPvp::SetSpawnWeapons(class CCharacter *pChr)
{
	switch(CGameControllerPvp::GetSpawnWeapons(pChr->GetPlayer()->GetCid()))
	{
	case SPAWN_WEAPON_LASER:
		pChr->GiveWeapon(WEAPON_LASER, false);
		break;
	case SPAWN_WEAPON_GRENADE:
		pChr->GiveWeapon(WEAPON_GRENADE, false, g_Config.m_SvGrenadeAmmoRegen ? g_Config.m_SvGrenadeAmmoRegenNum : -1);
		break;
	default:
		dbg_msg("zcatch", "invalid sv_spawn_weapons");
		break;
	}
}
int CGameControllerPvp::GetDefaultWeaponBasedOnSpawnWeapons() const
{
	switch(m_SpawnWeapons)
	{
	case SPAWN_WEAPON_LASER:
		return WEAPON_LASER;
	case SPAWN_WEAPON_GRENADE:
		return WEAPON_GRENADE;
	default:
		dbg_msg("zcatch", "invalid sv_spawn_weapons");
		break;
	}
	return WEAPON_GUN;
}

bool CGameControllerPvp::CanSpawn(int Team, vec2 *pOutPos, int DDTeam)
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	CSpawnEval Eval;
	if(IsTeamPlay()) // ddnet-insta
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(&Eval, 1 + (Team & 1), DDTeam);
		if(!Eval.m_Got)
		{
			EvaluateSpawnType(&Eval, 0, DDTeam);
			if(!Eval.m_Got)
				EvaluateSpawnType(&Eval, 1 + ((Team + 1) & 1), DDTeam);
		}
	}
	else
	{
		EvaluateSpawnType(&Eval, 0, DDTeam);
		EvaluateSpawnType(&Eval, 1, DDTeam);
		EvaluateSpawnType(&Eval, 2, DDTeam);
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

void CGameControllerPvp::OnCharacterSpawn(class CCharacter *pChr)
{
	OnCharacterConstruct(pChr);

	pChr->SetTeams(&Teams());
	Teams().OnCharacterSpawn(pChr->GetPlayer()->GetCid());

	// default health
	pChr->IncreaseHealth(10);

	pChr->GetPlayer()->UpdateLastToucher(-1);

	if(pChr->GetPlayer()->m_FreezeOnSpawn)
	{
		pChr->Freeze(pChr->GetPlayer()->m_FreezeOnSpawn);
		pChr->GetPlayer()->m_FreezeOnSpawn = 0;

		char aBuf[512];
		str_format(
			aBuf,
			sizeof(aBuf),
			"'%s' spawned frozen because he quit while being frozen",
			Server()->ClientName(pChr->GetPlayer()->GetCid()));
		SendChat(-1, TEAM_ALL, aBuf);
	}
}

void CGameControllerPvp::AddSpree(class CPlayer *pPlayer)
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

void CGameControllerPvp::EndSpree(class CPlayer *pPlayer, class CPlayer *pKiller)
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

void CGameControllerPvp::OnLoadedNameStats(const CSqlStatsPlayer *pStats, class CPlayer *pPlayer)
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

bool CGameControllerPvp::LoadNewPlayerNameData(int ClientId)
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

void CGameControllerPvp::OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData)
{
}

void CGameControllerPvp::OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData)
{
}

bool CGameControllerPvp::OnSkinChange7(protocol7::CNetMsg_Cl_SkinChange *pMsg, int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];

	CTeeInfo Info(pMsg->m_apSkinPartNames, pMsg->m_aUseCustomColors, pMsg->m_aSkinPartColors);
	Info.FromSixup();

	CTeeInfo OldInfo = pPlayer->m_TeeInfos;
	pPlayer->m_TeeInfos = Info;

	// restore old color
	if(!IsSkinColorChangeAllowed())
	{
		for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
		{
			pPlayer->m_TeeInfos.m_aSkinPartColors[p] = OldInfo.m_aSkinPartColors[p];
			pPlayer->m_TeeInfos.m_aUseCustomColors[p] = OldInfo.m_aUseCustomColors[p];
		}
	}

	protocol7::CNetMsg_Sv_SkinChange Msg;
	Msg.m_ClientId = ClientId;
	for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
	{
		Msg.m_apSkinPartNames[p] = pPlayer->m_TeeInfos.m_apSkinPartNames[p];
		Msg.m_aSkinPartColors[p] = pPlayer->m_TeeInfos.m_aSkinPartColors[p];
		Msg.m_aUseCustomColors[p] = pPlayer->m_TeeInfos.m_aUseCustomColors[p];
	}

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
	return true;
}

bool CGameControllerPvp::OnTeamChatCmd(IConsole::IResult *pResult)
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

bool CGameControllerPvp::OnSetDDRaceTeam(int ClientId, int Team)
{
	// only joining team 0
	// forces players to spectators
	// to avoid players interrrupting gameplay
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

	// set m_Team directly to avoid recursive loop
	// we do not update the team size because this is not t0
	// and later we again call SetTeam which sends the team change
	// net message so clients are aware of the correct team
	//
	// TODO: revist this and check if 0.7 works correctly
	//       see https://github.com/ddnet-insta/ddnet-insta/issues/362
	pPlayer->SetTeamRawAndUnsafe(TEAM_SPECTATORS);

	CCharacter *pChr = GameServer()->GetPlayerChar(ClientId);
	if(pChr && pChr->IsAlive())
	{
		// the /team chat command is block for in game players
		// so only death should move you to team 0
		// any other cases is unexpected and should be investigated.
		//
		// ideally this branch is never hit because then some assumption is wrong
		// but we might be able to recover using SetTeam so this is not an assert
		log_error(
			"ddnet-insta",
			"cid=%d changed from ddrace team %d to ddrace team 0 but is still alive",
			ClientId,
			OldDDRaceTeam);
		pPlayer->SetTeam(TEAM_SPECTATORS);
	}
	else
	{
		// this is the expected branch
		// the players death triggerd the team change
		// so we have to use the NoKill version of set team
		// otherwise the player is killed twice
		// which causes nullptr issues
		pPlayer->SetTeamNoKill(TEAM_SPECTATORS);
		SendChatTarget(ClientId, "You were forced to spectators because you left the ddrace team. Use /lock to stay in game");
	}

	return false;
}

void CGameControllerPvp::OnPlayerConnect(CPlayer *pPlayer)
{
	m_InvalidateConnectedIpsCache = true;
	IGameController::OnPlayerConnect(pPlayer);
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
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		if(!g_Config.m_SvTournamentJoinMsgs || pPlayer->GetTeam() != TEAM_SPECTATORS)
			GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);
		else if(g_Config.m_SvTournamentJoinMsgs == 2)
			SendChatSpectators(aBuf, CGameContext::FLAG_SIX);

		GameServer()->SendChatTarget(ClientId, "DDNet-insta " DDNET_INSTA_VERSIONSTR " github.com/ddnet-insta/ddnet-insta");
		GameServer()->SendChatTarget(ClientId, "DDraceNetwork Mod. Version: " GAME_VERSION);

		GameServer()->AlertOnSpecialInstagibConfigs(ClientId);
		GameServer()->ShowCurrentInstagibConfigsMotd(ClientId);
	}

	if((Server()->Tick() - GameServer()->m_NonEmptySince) / Server()->TickSpeed() < 20)
	{
		pPlayer->m_VerifiedForChat = true;
	}

	CheckReadyStates();
	SendGameInfo(ClientId); // update game info
	RestoreFreezeStateOnRejoin(pPlayer);
}

void CGameControllerPvp::RestoreFreezeStateOnRejoin(CPlayer *pPlayer)
{
	const NETADDR *pAddr = Server()->ClientAddr(pPlayer->GetCid());

	bool Match = false;
	int Index = -1;
	for(const auto &Quitter : m_vFrozenQuitters)
	{
		Index++;
		if(!net_addr_comp_noport(&Quitter, pAddr))
		{
			Match = true;
			break;
		}
	}

	if(Match)
	{
		log_info("ddnet-insta", "a frozen player rejoined removing slot %d (%zu left)", Index, m_vFrozenQuitters.size() - 1);
		m_vFrozenQuitters.erase(m_vFrozenQuitters.begin() + Index);

		pPlayer->m_FreezeOnSpawn = 20;
	}
}

void CGameControllerPvp::SendChatSpectators(const char *pMessage, int Flags)
{
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() != TEAM_SPECTATORS)
			continue;
		bool Send = (Server()->IsSixup(pPlayer->GetCid()) && (Flags & CGameContext::FLAG_SIXUP)) ||
			    (!Server()->IsSixup(pPlayer->GetCid()) && (Flags & CGameContext::FLAG_SIX));
		if(!Send)
			continue;

		GameServer()->SendChat(pPlayer->GetCid(), TEAM_ALL, pMessage, -1, Flags);
	}
}

void CGameControllerPvp::OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason)
{
	if(GameState() != IGS_END_ROUND)
		SaveStatsOnDisconnect(pPlayer);

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		if(GameServer()->GetDDRaceTeam(pPlayer->GetCid()) == 0)
			--m_aTeamSize[pPlayer->GetTeam()];
		m_UnbalancedTick = TBALANCE_CHECK;
	}

	while(true)
	{
		if(!g_Config.m_SvPunishFreezeDisconnect)
			break;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			break;
		if(!pChr->m_FreezeTime)
			break;

		const NETADDR *pAddr = Server()->ClientAddr(pPlayer->GetCid());
		m_vFrozenQuitters.emplace_back(*pAddr);

		// frozen quit punishment expires after 5 minutes
		// to avoid memory leaks
		m_ReleaseAllFrozenQuittersTick = Server()->Tick() + Server()->TickSpeed() * 300;
		break;
	}

	m_InvalidateConnectedIpsCache = true;
	pPlayer->OnDisconnect();
	int ClientId = pPlayer->GetCid();
	if(Server()->ClientIngame(ClientId))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(ClientId), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(ClientId));
		if(!g_Config.m_SvTournamentJoinMsgs || pPlayer->GetTeam() != TEAM_SPECTATORS)
			GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);
		else if(g_Config.m_SvTournamentJoinMsgs == 2)
			SendChatSpectators(aBuf, CGameContext::FLAG_SIX);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientId, Server()->ClientName(ClientId));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}

	CheckReadyStates(ClientId);
}

void CGameControllerPvp::DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	// has to be saved for later
	// because the set team operation kills the character
	// and then we lose the team information
	int DDRaceTeam = GameServer()->GetDDRaceTeam(pPlayer->GetCid());

	Team = ClampTeam(Team);
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

void CGameControllerPvp::Anticamper()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();

		//Dont do anticamper if there is no character
		if(!pChr)
		{
			pPlayer->m_CampTick = -1;
			pPlayer->m_SentCampMsg = false;
			continue;
		}

		//Dont do anticamper if player is already frozen
		if(pChr->m_FreezeTime > 0 || pChr->GetCore().m_DeepFrozen)
		{
			pPlayer->m_CampTick = -1;
			pPlayer->m_SentCampMsg = false;
			continue;
		}

		int AnticamperTime = g_Config.m_SvAnticamperTime;
		int AnticamperRange = g_Config.m_SvAnticamperRange;

		if(pPlayer->m_CampTick == -1)
		{
			pPlayer->m_CampPos = pChr->m_Pos;
			pPlayer->m_CampTick = Server()->Tick() + Server()->TickSpeed() * AnticamperTime;
		}

		// Check if the player is moving
		if((pPlayer->m_CampPos.x - pChr->m_Pos.x >= (float)AnticamperRange || pPlayer->m_CampPos.x - pChr->m_Pos.x <= -(float)AnticamperRange) || (pPlayer->m_CampPos.y - pChr->m_Pos.y >= (float)AnticamperRange || pPlayer->m_CampPos.y - pChr->m_Pos.y <= -(float)AnticamperRange))
		{
			pPlayer->m_CampTick = -1;
			pPlayer->m_SentCampMsg = false;
		}

		// Send warning to the player
		if(pPlayer->m_CampTick <= Server()->Tick() + Server()->TickSpeed() * 5 && pPlayer->m_CampTick != -1 && !pPlayer->m_SentCampMsg)
		{
			GameServer()->SendBroadcast("ANTICAMPER: Move or die", pPlayer->GetCid());
			pPlayer->m_SentCampMsg = true;
		}

		// Kill him
		if((pPlayer->m_CampTick <= Server()->Tick()) && (pPlayer->m_CampTick > 0))
		{
			if(g_Config.m_SvAnticamperFreeze)
			{
				//Freeze player
				pChr->Freeze(g_Config.m_SvAnticamperFreeze);
				GameServer()->CreateSound(pChr->m_Pos, SOUND_PLAYER_PAIN_LONG);

				//Reset anticamper
				pPlayer->m_CampTick = -1;
				pPlayer->m_SentCampMsg = false;

				continue;
			}
			else
			{
				//Kill Player
				pChr->Die(pPlayer->GetCid(), WEAPON_WORLD);

				//Reset counter on death
				pPlayer->m_CampTick = -1;
				pPlayer->m_SentCampMsg = false;

				continue;
			}
		}
	}
}

bool CGameControllerPvp::BlockFirstShotOnSpawn(class CCharacter *pChr, int Weapon) const
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
	// except the ones that activate full auto for certain weapons
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

bool CGameControllerPvp::OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos)
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

	if(IsStatTrack() && Weapon != WEAPON_HAMMER)
		Character.GetPlayer()->m_Stats.m_ShotsFired++;

	if(g_Config.m_SvGrenadeAmmoRegenResetOnFire)
		Character.m_Core.m_aWeapons[Character.m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
	if(Character.m_Core.m_aWeapons[Character.m_Core.m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		Character.m_Core.m_aWeapons[Character.m_Core.m_ActiveWeapon].m_Ammo--;

	if(Weapon == WEAPON_HAMMER)
	{
		// reset objects Hit
		Character.m_NumObjectsHit = 0;
		GameServer()->CreateSound(Character.m_Pos, SOUND_HAMMER_FIRE, Character.TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)

		Character.Antibot()->OnHammerFire(Character.m_pPlayer->GetCid());

		if(!Character.m_Core.m_HammerHitDisabled)
		{
			CEntity *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_Rollback.FindCharactersOnTick(ProjStartPos, Character.GetProximityRadius() * 0.5f, apEnts,
				MAX_CLIENTS, Character.m_pPlayer->m_LastAckedSnapshot);

			for(int i = 0; i < Num; ++i)
			{
				auto *pTarget = static_cast<CCharacter *>(apEnts[i]);

				//if ((pTarget == this) || Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
				if((pTarget == Character.m_pPlayer->GetCharacter() || (pTarget->IsAlive() && !Character.CanCollide(pTarget->GetPlayer()->GetCid()))))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos - ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * Character.GetProximityRadius() * 0.5f, Character.TeamMask());
				else
					GameServer()->CreateHammerHit(ProjStartPos, Character.TeamMask());

				vec2 Dir;
				if(length(pTarget->m_Pos - Character.m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - Character.m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				float Strength = Character.GetTuning(Character.m_TuneZone)->m_HammerStrength;

				vec2 Temp = pTarget->m_Core.m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
				Temp = ClampVel(pTarget->m_MoveRestrictions, Temp);
				Temp -= pTarget->m_Core.m_Vel;
				pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
					Character.m_pPlayer->GetCid(), Character.m_Core.m_ActiveWeapon);

				if(!GameServer()->m_pController->IsFngGameType())
					pTarget->UnFreeze();

				Character.Antibot()->OnHammerHit(Character.m_pPlayer->GetCid(), pTarget->GetPlayer()->GetCid());

				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
			{
				float FireDelay = Character.GetTuning(Character.m_TuneZone)->m_HammerHitFireDelay;
				Character.m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
			}
		}
	}
	else if(Weapon == WEAPON_GUN)
	{
		if(!Character.m_Core.m_Jetpack || !Character.m_pPlayer->m_NinjaJetpack || Character.m_Core.m_HasTelegunGun)
		{
			int Lifetime = (int)(Server()->TickSpeed() * Character.GetTuning(Character.m_TuneZone)->m_GunLifetime);

			new CPvpProjectile(
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
			float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

			// TODO: not sure about Dir and InitDir and prediction

			new CPvpProjectile(
				Character.GameWorld(),
				WEAPON_SHOTGUN, // Type
				Character.GetPlayer()->GetCid(), // Owner
				ProjStartPos, // Pos
				direction(Angle) * Speed, // Dir
				(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime), // Span
				false, // Freeze
				false, // Explosive
				-1, // SoundImpact
				vec2(cosf(Angle), sinf(Angle)) * Speed); // InitDir
		}

		GameServer()->CreateSound(Character.m_Pos, SOUND_SHOTGUN_FIRE); // NOLINT(clang-analyzer-unix.Malloc)
	}
	else if(Weapon == WEAPON_GRENADE)
	{
		int Lifetime = (int)(Server()->TickSpeed() * Character.GetTuning(Character.m_TuneZone)->m_GrenadeLifetime);

		new CPvpProjectile(
			Character.GameWorld(),
			WEAPON_GRENADE, //Type
			Character.m_pPlayer->GetCid(), //Owner
			ProjStartPos, //Pos
			Direction, //Dir
			Lifetime, //Span
			false, //Freeze
			true, //Explosive
			SOUND_GRENADE_EXPLODE, //SoundImpact
			MouseTarget // MouseTarget
		);

		GameServer()->CreateSound(Character.m_Pos, SOUND_GRENADE_FIRE, Character.TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	else if(Weapon == WEAPON_LASER)
	{
		float LaserReach = Character.GetTuning(Character.m_TuneZone)->m_LaserReach;

		new CPvpLaser(Character.GameWorld(), Character.m_Pos, Direction, LaserReach, Character.m_pPlayer->GetCid(), WEAPON_LASER);
		GameServer()->CreateSound(Character.m_Pos, SOUND_LASER_FIRE, Character.TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	else
	{
		return false;
	}

	Character.m_AttackTick = Server()->Tick();

	if(!Character.m_ReloadTimer)
	{
		float FireDelay;
		Character.GetTuning(Character.m_TuneZone)->Get(38 + Character.m_Core.m_ActiveWeapon, &FireDelay);
		Character.m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
	}

	return true;
}

void CGameControllerPvp::DoDamageHitSound(int KillerId)
{
	if(KillerId < 0 || KillerId >= MAX_CLIENTS)
		return;
	CPlayer *pKiller = GameServer()->m_apPlayers[KillerId];
	if(!pKiller)
		return;

	// do damage Hit sound
	CClientMask Mask = CClientMask().set(KillerId);
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;

		if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorId == KillerId)
			Mask.set(i);
	}
	GameServer()->CreateSound(pKiller->m_ViewPos, SOUND_HIT, Mask);
}

void CGameControllerPvp::MakeLaserTextPoints(vec2 Pos, int Points, int Seconds)
{
	if(!g_Config.m_SvLaserTextPoints)
		return;

	char aText[16];
	if(Points >= 0)
		str_format(aText, sizeof(aText), "+%d", Points);
	else
		str_format(aText, sizeof(aText), "%d", Points);
	Pos.y -= 60.0f;
	new CLaserText(&GameServer()->m_World, Pos, Server()->TickSpeed() * Seconds, aText);
} // NOLINT(clang-analyzer-unix.Malloc)

int CGameControllerPvp::NumConnectedIps()
{
	if(!m_InvalidateConnectedIpsCache)
		return m_NumConnectedIpsCached;

	m_InvalidateConnectedIpsCache = false;
	m_NumConnectedIpsCached = Server()->DistinctClientCount();
	return m_NumConnectedIpsCached;
}

int CGameControllerPvp::NumActivePlayers()
{
	int Active = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && (pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_IsDead))
			Active++;
	return Active;
}

int CGameControllerPvp::NumAlivePlayers()
{
	int Alive = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetCharacter())
			Alive++;
	return Alive;
}

int CGameControllerPvp::NumNonDeadActivePlayers()
{
	int Alive = 0;
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && !pPlayer->m_IsDead && pPlayer->GetTeam() != TEAM_SPECTATORS)
			Alive++;
	return Alive;
}

int CGameControllerPvp::GetHighestSpreeClientId()
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

int CGameControllerPvp::GetFirstAlivePlayerId()
{
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetCharacter())
			return pPlayer->GetCid();
	return -1;
}

void CGameControllerPvp::KillAllPlayers()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->KillCharacter();
	}
}

CPlayer *CGameControllerPvp::GetPlayerByUniqueId(uint32_t UniqueId)
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetUniqueCid() == UniqueId)
			return pPlayer;
	return nullptr;
}

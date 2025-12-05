#include "insta_core.h"

#include <base/log.h>
#include <base/system.h>

#include <engine/server/server.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/race_state.h>
#include <game/server/entities/character.h>
#include <game/server/entities/ddnet_pvp/vanilla_projectile.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/antibob.h>
#include <game/server/instagib/enums.h>
#include <game/server/instagib/ip_storage.h>
#include <game/server/instagib/laser_text.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/structs.h>
#include <game/server/instagib/version.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/server/teams.h>
#include <game/teamscore.h>
#include <game/version.h>

CGameControllerInstaCore::CGameControllerInstaCore(class CGameContext *pGameServer) :
	CGameControllerDDRace(pGameServer)
{
	log_info("ddnet-insta", "initializing insta core ...");

	UpdateSpawnWeapons(true, true);
	m_vFrozenQuitters.clear();
	g_AntibobContext.m_pConsole = Console();
}

CGameControllerInstaCore::~CGameControllerInstaCore()
{
	log_info("ddnet-insta", "shutting down insta core ...");
}

void CGameControllerInstaCore::SendChatTarget(int To, const char *pText, int Flags) const
{
	GameServer()->SendChatTarget(To, pText, Flags);
}

void CGameControllerInstaCore::SendChat(int ClientId, int Team, const char *pText, int SpamProtectionClientId, int Flags)
{
	GameServer()->SendChat(ClientId, Team, pText, SpamProtectionClientId, Flags);
}

void CGameControllerInstaCore::SendChatSpectators(const char *pMessage, int Flags)
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

void CGameControllerInstaCore::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	// fallback to project wide credits
	// if the mode did not set specific ones
	// but it is recommended that every mode defines their own credits
	// these project wide credits can be fetched with "/credits_insta"
	GameServer()->PrintInstaCredits();
}

void CGameControllerInstaCore::OnReset()
{
	CGameControllerDDRace::OnReset();

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_IsReadyToPlay = true;
		pPlayer->m_ScoreStartTick = Server()->Tick();
	}
}

void CGameControllerInstaCore::OnInit()
{
}

void CGameControllerInstaCore::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	m_InvalidateConnectedIpsCache = true;

	int ClientId = pPlayer->GetCid();
	CIpStorage *pIpStorage = GameServer()->m_IpStorageController.FindEntry(Server()->ClientAddr(ClientId));
	if(pIpStorage)
	{
		char aAddr[512];
		net_addr_str(Server()->ClientAddr(ClientId), aAddr, sizeof(aAddr), false);
		log_info(
			"ddnet-insta",
			"player cid=%d name='%s' ip=%s loaded ip storage (in total there are %" PRIzu " entries)",
			ClientId,
			Server()->ClientName(ClientId),
			aAddr,
			GameServer()->m_IpStorageController.Entries().size());
		pPlayer->m_IpStorage = *pIpStorage;
	}

	if((Server()->Tick() - GameServer()->m_NonEmptySince) / Server()->TickSpeed() < 20)
	{
		pPlayer->m_VerifiedForChat = true;
	}

	RestoreFreezeStateOnRejoin(pPlayer);
	PrintConnect(pPlayer, Server()->ClientName(pPlayer->GetCid()));
	if(!Server()->ClientPrevIngame(ClientId))
	{
		PrintModWelcome(pPlayer);
	}
}

// this method should be kept as slim as possible in insta core
// all logic should be moved to InstaCoreDisconnect
// so controllers inheriting can easier reimplement parts they want
void CGameControllerInstaCore::OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason)
{
	InstaCoreDisconnect(pPlayer, pReason);
	pPlayer->OnDisconnect();
	PrintDisconnect(pPlayer, pReason);
}

// Holds all core logic. Should not contain code that can be possibly unwanted
// by custom controllers inheriting.
// Code that other controllers might want to change or drop should go into
// extra methods such as PrintDisconnect
void CGameControllerInstaCore::InstaCoreDisconnect(CPlayer *pPlayer, const char *pReason)
{
	m_InvalidateConnectedIpsCache = true;

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

	if(pPlayer->m_IpStorage.has_value() && !pPlayer->m_IpStorage.value().IsEmpty(Server()->Tick()))
	{
		const NETADDR *pAddr = Server()->ClientAddr(pPlayer->GetCid());
		CIpStorage *pStorage = GameServer()->m_IpStorageController.FindOrCreateEntry(pAddr, Server()->ClientName(pPlayer->GetCid()));
		pStorage->OnPlayerDisconnect(&pPlayer->m_IpStorage.value(), Server()->Tick());
	}
}

void CGameControllerInstaCore::PrintDisconnect(CPlayer *pPlayer, const char *pReason)
{
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
}

void CGameControllerInstaCore::PrintConnect(CPlayer *pPlayer, const char *pName)
{
	int ClientId = pPlayer->GetCid();
	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		// you could also use Server()->ClientName(ClientId)
		// instead of pName
		// but if accounts and locked names are enabled they might be different
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", pName, GetTeamName(pPlayer->GetTeam()));
		if(!g_Config.m_SvTournamentJoinMsgs || pPlayer->GetTeam() != TEAM_SPECTATORS)
			GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);
		else if(g_Config.m_SvTournamentJoinMsgs == 2)
			SendChatSpectators(aBuf, CGameContext::FLAG_SIX);
	}
}

void CGameControllerInstaCore::PrintModWelcome(CPlayer *pPlayer)
{
	int ClientId = pPlayer->GetCid();
	GameServer()->SendChatTarget(ClientId, "DDNet-insta " DDNET_INSTA_VERSIONSTR " github.com/ddnet-insta/ddnet-insta");
	GameServer()->SendChatTarget(ClientId, "DDraceNetwork Mod. Version: " GAME_VERSION);
}

void CGameControllerInstaCore::OnCharacterSpawn(class CCharacter *pChr)
{
	CPlayer *pPlayer = pChr->GetPlayer();
	pChr->m_IsGodmode = false;

	pChr->SetTeams(&Teams());
	Teams().OnCharacterSpawn(pPlayer->GetCid());

	// default health
	pChr->IncreaseHealth(10);

	pPlayer->UpdateLastToucher(-1, -1);

	if(pPlayer->m_IpStorage.has_value() && pPlayer->m_IpStorage.value().DeepUntilTick() > Server()->Tick())
	{
		pChr->SetDeepFrozen(true);
	}
	else if(pPlayer->m_FreezeOnSpawn)
	{
		pChr->Freeze(pPlayer->m_FreezeOnSpawn);
		pPlayer->m_FreezeOnSpawn = 0;

		char aBuf[512];
		str_format(
			aBuf,
			sizeof(aBuf),
			"'%s' spawned frozen because he quit while being frozen",
			Server()->ClientName(pPlayer->GetCid()));
		SendChat(-1, TEAM_ALL, aBuf);
	}
}

int CGameControllerInstaCore::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
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
	return 0;
}

void CGameControllerInstaCore::Tick()
{
	CGameControllerDDRace::Tick();
	GameServer()->m_IpStorageController.OnTick(Server()->Tick());

	if(m_TicksUntilShutdown)
	{
		m_TicksUntilShutdown--;
		if(m_TicksUntilShutdown < 1)
		{
			Server()->ShutdownServer();
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

		// ratelimit the 0.7 stuff because it requires net messages
		if(Server()->Tick() % 4 == 0)
		{
			if(pPlayer->m_SkinInfoManager.NeedsNetMessage7())
			{
				pPlayer->m_SkinInfoManager.OnSendNetMessage7();
				CTeeInfo TeeInfo = pPlayer->m_SkinInfoManager.TeeInfo();
				protocol7::CNetMsg_Sv_SkinChange Msg;
				Msg.m_ClientId = pPlayer->GetCid();
				for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
				{
					Msg.m_apSkinPartNames[p] = TeeInfo.m_aaSkinPartNames[p];
					Msg.m_aSkinPartColors[p] = TeeInfo.m_aSkinPartColors[p];
					Msg.m_aUseCustomColors[p] = TeeInfo.m_aUseCustomColors[p];
				}
				bool NetworkClip = pPlayer->GetCharacter()->HasRainbow();
				SendSkinChangeToAllSixup(&Msg, pPlayer, NetworkClip);
			}
		}
	}

	if(g_Config.m_SvAnticamper && !GameServer()->m_World.m_Paused)
		Anticamper();

	if(m_ReleaseAllFrozenQuittersTick < Server()->Tick() && !m_vFrozenQuitters.empty())
	{
		log_info("ddnet-insta", "all freeze quitter punishments expired. cleaning up ...");
		m_vFrozenQuitters.clear();
	}
}

bool CGameControllerInstaCore::OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId)
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
bool CGameControllerInstaCore::OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId)
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

int CGameControllerInstaCore::GetPlayerTeam(class CPlayer *pPlayer, bool Sixup)
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

int CGameControllerInstaCore::GetAutoTeam(int NotThisId)
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

bool CGameControllerInstaCore::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
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

bool CGameControllerInstaCore::IsValidTeam(int Team)
{
	if(IsTeamPlay())
	{
		if(Team == TEAM_RED)
			return true;
		if(Team == TEAM_BLUE)
			return true;
	}
	return IGameController::IsValidTeam(Team);
}

const char *CGameControllerInstaCore::GetTeamName(int Team)
{
	if(IsTeamPlay())
	{
		if(Team == TEAM_RED)
			return "red team";
		if(Team == TEAM_BLUE)
			return "blue team";
	}

	return IGameController::GetTeamName(Team);
}

bool CGameControllerInstaCore::CanSpawn(int Team, vec2 *pOutPos, int DDTeam)
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	CSpawnEval Eval;
	if(IsTeamPlay()) // ddnet-insta
	{
		Eval.m_FriendlyTeam = Team;

		// first try own team spawn, then normal spawn and then enemy
		EvaluateSpawnType(&Eval, (ESpawnType)(1 + (Team & 1)), DDTeam);
		if(!Eval.m_Got)
		{
			EvaluateSpawnType(&Eval, SPAWNTYPE_DEFAULT, DDTeam);
			if(!Eval.m_Got)
				EvaluateSpawnType(&Eval, (ESpawnType)(1 + ((Team + 1) & 1)), DDTeam);
		}
	}
	else
	{
		EvaluateSpawnType(&Eval, SPAWNTYPE_DEFAULT, DDTeam);
		EvaluateSpawnType(&Eval, SPAWNTYPE_RED, DDTeam);
		EvaluateSpawnType(&Eval, SPAWNTYPE_BLUE, DDTeam);
	}

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

bool CGameControllerInstaCore::OnSkinChange7(protocol7::CNetMsg_Cl_SkinChange *pMsg, int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];

	// parse 0.7 info
	pPlayer->m_TeeInfos = CTeeInfo(pMsg->m_apSkinPartNames, pMsg->m_aUseCustomColors, pMsg->m_aSkinPartColors);
	pPlayer->m_TeeInfos.FromSixup();

	// store user request
	pPlayer->m_SkinInfoManager.SetUserChoice(pPlayer->m_TeeInfos);

	// enforce server set info
	pPlayer->m_TeeInfos = pPlayer->m_SkinInfoManager.TeeInfo();

	protocol7::CNetMsg_Sv_SkinChange Msg;
	Msg.m_ClientId = ClientId;
	for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
	{
		Msg.m_apSkinPartNames[p] = pPlayer->m_TeeInfos.m_aaSkinPartNames[p];
		Msg.m_aSkinPartColors[p] = pPlayer->m_TeeInfos.m_aSkinPartColors[p];
		Msg.m_aUseCustomColors[p] = pPlayer->m_TeeInfos.m_aUseCustomColors[p];
	}

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
	return true;
}

void CGameControllerInstaCore::OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData)
{
}

void CGameControllerInstaCore::OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData)
{
}

// called on round init and on join
void CGameControllerInstaCore::RoundInitPlayer(CPlayer *pPlayer)
{
	pPlayer->m_IsDead = false;
	pPlayer->m_KillerId = -1;
}

// this is only called once on connect
// NOT ON ROUND END
void CGameControllerInstaCore::InitPlayer(CPlayer *pPlayer)
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

void CGameControllerInstaCore::Snap(int SnappingClient)
{
	CGameControllerDDRace::Snap(SnappingClient);

	// it is a bit of a mess that 0.6 has both teamscore
	// and flag carriers in one object
	// so the only way to have consistent 0.6 and 0.7 behavior is to
	// regress 0.7 to also send teamscore together with the flag carriers
	// even if it could split it
	//
	// But that is fine. We could also not check teamplay or gameflag_flags at all
	// both the 0.6 and 0.7 client handle receiving these snap objects well.
	// Even if the gametype is dm it would just ignore the teamscore.
	// I still decided to require either teamplay or gameflag_flags
	// to have a bit smaller snap size in modes that do not use either of the two.
	if(IsTeamPlay() || (m_GameFlags & GAMEFLAG_FLAGS))
	{
		if(Server()->IsSixup(SnappingClient))
		{
			protocol7::CNetObj_GameDataFlag *pGameDataObj = Server()->SnapNewItem<protocol7::CNetObj_GameDataFlag>(0);
			if(!pGameDataObj)
				return;

			pGameDataObj->m_FlagCarrierRed = SnapFlagCarrierRed(SnappingClient);
			pGameDataObj->m_FlagCarrierBlue = SnapFlagCarrierBlue(SnappingClient);

			protocol7::CNetObj_GameDataTeam *pGameDataTeam = Server()->SnapNewItem<protocol7::CNetObj_GameDataTeam>(0);
			if(!pGameDataTeam)
				return;

			pGameDataTeam->m_TeamscoreRed = SnapTeamscoreRed(SnappingClient);
			pGameDataTeam->m_TeamscoreBlue = SnapTeamscoreBlue(SnappingClient);
		}
		else
		{
			CNetObj_GameData *pGameDataObj = Server()->SnapNewItem<CNetObj_GameData>(0);
			if(!pGameDataObj)
				return;

			pGameDataObj->m_FlagCarrierRed = SnapFlagCarrierRed(SnappingClient);
			pGameDataObj->m_FlagCarrierBlue = SnapFlagCarrierBlue(SnappingClient);

			pGameDataObj->m_TeamscoreRed = SnapTeamscoreRed(SnappingClient);
			pGameDataObj->m_TeamscoreBlue = SnapTeamscoreBlue(SnappingClient);
		}
	}
}

int CGameControllerInstaCore::SnapFlagCarrierRed(int SnappingClient)
{
	int FlagCarrierRed = FLAG_MISSING;
	if(m_apFlags[TEAM_RED])
	{
		if(m_apFlags[TEAM_RED]->m_AtStand)
			FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->GetCarrier() && m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer())
			FlagCarrierRed = m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierRed = FLAG_TAKEN;
	}
	return FlagCarrierRed;
}

int CGameControllerInstaCore::SnapFlagCarrierBlue(int SnappingClient)
{
	int FlagCarrierBlue = FLAG_MISSING;
	if(m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_BLUE]->m_AtStand)
			FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_BLUE]->GetCarrier() && m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer())
			FlagCarrierBlue = m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierBlue = FLAG_TAKEN;
	}
	return FlagCarrierBlue;
}

int CGameControllerInstaCore::SnapTeamscoreRed(int SnappingClient)
{
	return m_aTeamscore[TEAM_RED];
}

int CGameControllerInstaCore::SnapTeamscoreBlue(int SnappingClient)
{
	return m_aTeamscore[TEAM_BLUE];
}

int CGameControllerInstaCore::SnapPlayerFlags7(int SnappingClient, CPlayer *pPlayer, int PlayerFlags7)
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

void CGameControllerInstaCore::SnapPlayer6(int SnappingClient, CPlayer *pPlayer, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo)
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
		StrToInts(pClientInfo->m_aName, std::size(pClientInfo->m_aName), aReady);
	}

	// TODO: can we save clock cycles here?
	//       maybe only set it if the skin info manager has custom values
	//       something like if(pPlayer->m_SkinInfoManager.HasValues())
	//       which is just a cheap bool lookup
	CTeeInfo Info = pPlayer->m_SkinInfoManager.TeeInfo();
	StrToInts(pClientInfo->m_aSkin, std::size(pClientInfo->m_aSkin), Info.m_aSkinName);
	pClientInfo->m_UseCustomColor = Info.m_UseCustomColor;
	pClientInfo->m_ColorBody = Info.m_ColorBody;
	pClientInfo->m_ColorFeet = Info.m_ColorFeet;
}

void CGameControllerInstaCore::SnapDDNetPlayer(int SnappingClient, CPlayer *pPlayer, CNetObj_DDNetPlayer *pDDNetPlayer)
{
	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return;

	if(g_Config.m_SvHideAdmins && Server()->GetAuthedState(SnappingClient) == AUTHED_NO)
		pDDNetPlayer->m_AuthLevel = AUTHED_NO;
}

bool CGameControllerInstaCore::OnClientPacket(int ClientId, bool Sys, int MsgId, CNetChunk *pPacket, CUnpacker *pUnpacker)
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

bool CGameControllerInstaCore::UnfreezeOnHammerHit() const
{
	if(IsFngGameType())
		return false;
	return g_Config.m_SvFreezeHammer == 0;
}

void CGameControllerInstaCore::OnPlayerTick(class CPlayer *pPlayer)
{
	pPlayer->InstagibTick();

	if(pPlayer->m_ForceTeam.m_Tick > 0 && Server()->Tick() > pPlayer->m_ForceTeam.m_Tick)
	{
		int WantedTeam = pPlayer->m_ForceTeam.m_Team;
		if(pPlayer->GetTeam() != WantedTeam)
			pPlayer->SetTeam(WantedTeam);
		if(WantedTeam == TEAM_SPECTATORS)
			pPlayer->SetSpectatorId(pPlayer->m_ForceTeam.m_SpectatorId);
		pPlayer->m_ForceTeam.m_Tick = 0;
	}

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
				pHooked->UpdateLastToucher(pChr->GetPlayer()->GetCid(), WEAPON_HOOK);
			}
		}
	}
}

void CGameControllerInstaCore::OnCharacterTick(CCharacter *pChr)
{
	if(pChr->GetPlayer()->m_PlayerFlags & PLAYERFLAG_CHATTING)
		pChr->GetPlayer()->m_TicksSpentChatting++;
}

void CGameControllerInstaCore::SendSkinChangeToAllSixup(protocol7::CNetMsg_Sv_SkinChange *pMsg, CPlayer *pPlayer, bool ApplyNetworkClipping)
{
	if(!pPlayer->GetCharacter())
		return;

	if(!ApplyNetworkClipping)
	{
		Server()->SendPackMsg(pMsg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
		return;
	}

	for(CPlayer *pReceivingPlayer : GameServer()->m_apPlayers)
	{
		if(!pReceivingPlayer)
			continue;
		if(!Server()->IsSixup(pReceivingPlayer->GetCid()))
			continue;

		const bool IsTopscorer = !GameServer()->m_pController->IsTeamPlay() && GameServer()->m_pController->HasWinningScore(pPlayer);

		// never clip when in scoreboard or the top scorer
		// to see the rainbow in scoreboard and hud in the bottom right
		if(!(pReceivingPlayer->m_PlayerFlags & PLAYERFLAG_SCOREBOARD) && !IsTopscorer)
			if(NetworkClipped(GameServer(), pReceivingPlayer->GetCid(), pPlayer->GetCharacter()->GetPos()))
				continue;

		Server()->SendPackMsg(pMsg, MSGFLAG_VITAL | MSGFLAG_NORECORD, pReceivingPlayer->GetCid());
	}
}

void CGameControllerInstaCore::UpdateSpawnWeapons(bool Silent, bool Apply)
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
		log_warn("ddnet-insta", "WARNING: reload required for spawn weapons to apply");
	}
}

int CGameControllerInstaCore::GetDefaultWeaponBasedOnSpawnWeapons() const
{
	switch(m_SpawnWeapons)
	{
	case SPAWN_WEAPON_LASER:
		return WEAPON_LASER;
	case SPAWN_WEAPON_GRENADE:
		return WEAPON_GRENADE;
	default:
		dbg_assert_failed("Invalid m_SpawnWeapons: %d", m_SpawnWeapons);
	}
	return WEAPON_GUN;
}

void CGameControllerInstaCore::SetSpawnWeapons(class CCharacter *pChr)
{
	int SpawnWeapons = CGameControllerInstaCore::GetSpawnWeapons(pChr->GetPlayer()->GetCid());
	switch(SpawnWeapons)
	{
	case SPAWN_WEAPON_LASER:
		pChr->GiveWeapon(WEAPON_LASER, false);
		break;
	case SPAWN_WEAPON_GRENADE:
		pChr->GiveWeapon(WEAPON_GRENADE, false, g_Config.m_SvGrenadeAmmoRegen ? g_Config.m_SvGrenadeAmmoRegenNum : -1);
		break;
	default:
		dbg_assert_failed("Invalid SpawnWeapons: %d", SpawnWeapons);
	}
}

void CGameControllerInstaCore::OnUpdateSpectatorVotesConfig()
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
				log_error("ddnet-insta", "ERROR: tried to move player back to team=%d but expected spectators", pPlayer->GetTeam());
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

void CGameControllerInstaCore::RestoreFreezeStateOnRejoin(CPlayer *pPlayer)
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
		log_info("ddnet-insta", "a frozen player rejoined removing slot %d (%" PRIzu " left)", Index, m_vFrozenQuitters.size() - 1);
		m_vFrozenQuitters.erase(m_vFrozenQuitters.begin() + Index);

		pPlayer->m_FreezeOnSpawn = 20;
	}
}

void CGameControllerInstaCore::Anticamper()
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

void CGameControllerInstaCore::ApplyVanillaDamage(int &Dmg, int From, int Weapon, CCharacter *pCharacter)
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

void CGameControllerInstaCore::MakeLaserTextPoints(vec2 Pos, int Points, int Seconds, CClientMask Mask)
{
	if(!g_Config.m_SvLaserTextPoints)
		return;

	char aText[16];
	if(Points >= 0)
		str_format(aText, sizeof(aText), "+%d", Points);
	else
		str_format(aText, sizeof(aText), "%d", Points);
	Pos.y -= 60.0f;
	new CLaserText(&GameServer()->m_World, Pos, Server()->TickSpeed() * Seconds, aText, Mask);
} // NOLINT(clang-analyzer-unix.Malloc)

void CGameControllerInstaCore::DoDamageHitSound(int KillerId)
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

		if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->SpectatorId() == KillerId)
			Mask.set(i);
	}
	GameServer()->CreateSound(pKiller->m_ViewPos, SOUND_HIT, Mask);
}

void CGameControllerInstaCore::DoSpikeKillSound(int VictimId, int KillerId)
{
	if(VictimId < 0 || VictimId >= MAX_CLIENTS || KillerId < 0 || KillerId >= MAX_CLIENTS)
		return;
	auto *pVictim = GameServer()->m_apPlayers[VictimId];
	if(!pVictim)
		return;
	if(g_Config.m_SvSpikeSound == 1)
	{
		CClientMask Mask = CClientMask().set(KillerId);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i])
				continue;

			if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->SpectatorId() == KillerId)
				Mask.set(i);
		}
		const auto *pKiller = GameServer()->m_apPlayers[KillerId];
		GameServer()->CreateSound(pKiller ? pKiller->m_ViewPos : pVictim->m_ViewPos, SOUND_CTF_CAPTURE, Mask);
	}
	else if(g_Config.m_SvSpikeSound == 2)
	{
		auto *pVictimChr = pVictim->GetCharacter();
		if(pVictimChr)
		{
			CClientMask Mask = pVictimChr->TeamMask();
			Mask.reset(KillerId);
			GameServer()->CreateSound(pVictimChr->GetPos(), SOUND_CTF_GRAB_PL, Mask);
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE, KillerId);
		}
	}
}

int CGameControllerInstaCore::NumConnectedIps()
{
	if(!m_InvalidateConnectedIpsCache)
		return m_NumConnectedIpsCached;

	m_InvalidateConnectedIpsCache = false;
	m_NumConnectedIpsCached = Server()->DistinctClientCount();
	return m_NumConnectedIpsCached;
}

int CGameControllerInstaCore::GetFirstAlivePlayerId()
{
	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetCharacter())
			return pPlayer->GetCid();
	return -1;
}

void CGameControllerInstaCore::KillAllPlayers()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->KillCharacter();
	}
}

CPlayer *CGameControllerInstaCore::GetPlayerByUniqueId(uint32_t UniqueId)
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetUniqueCid() == UniqueId)
			return pPlayer;
	return nullptr;
}

#include <base/log.h>
#include <base/net.h>
#include <base/types.h>
#include <base/vmath.h>

#include <engine/antibot.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include <insta/server/ip_storage.h>

void CGameContext::ConHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_HAMMER, false);
}

void CGameContext::ConGun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GUN, false);
}

void CGameContext::ConUnHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_HAMMER, true);
}

void CGameContext::ConUnGun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GUN, true);
}

void CGameContext::ConGodmode(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CCharacter *pChr = pSelf->GetPlayerChar(Victim);

	if(!pChr)
		return;

	bool Give = pChr->m_IsGodmode = !pChr->m_IsGodmode;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' %s godmode!",
		pSelf->Server()->ClientName(Victim),
		Give ? "got" : "lost");
	pSelf->SendChat(-1, TEAM_ALL, aBuf);
}

void CGameContext::ConRainbow(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	CCharacter *pChr = pSelf->GetPlayerChar(Victim);

	if(!pChr)
		return;

	pChr->Rainbow(!pChr->HasRainbow());
}

void CGameContext::ConForceReady(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	if(Victim < 0 || Victim >= MAX_CLIENTS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "victim has to be in 0-64 range");
		return;
	}
	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;
	if(pPlayer->m_IsReadyToPlay)
		return;

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' was forced ready by an admin!",
			pSelf->Server()->ClientName(Victim));
		pSelf->SendChat(-1, TEAM_ALL, aBuf);
	}

	pPlayer->m_IsReadyToPlay = true;
	pSelf->PlayerReadyStateBroadcast();
	pSelf->m_pController->CheckReadyStates();
}

void CGameContext::ConChat(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(pResult->m_ClientId, TEAM_ALL, pResult->GetString(0));
}

void CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ShuffleTeams();
}

void CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
}

void CGameContext::ConSwapTeamsRandom(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(rand() % 2)
		pSelf->SwapTeams();
	else
		dbg_msg("swap", "did not swap due to random chance");
}

void CGameContext::ConForceTeamBalance(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	pSelf->m_pController->DoTeamBalance();
}

void CGameContext::ConAddPassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pPassword = pResult->GetString(0);

	if(std::find(pSelf->m_vPasswords.begin(), pSelf->m_vPasswords.end(), pPassword) != pSelf->m_vPasswords.end())
	{
		log_warn("server", "the password '%s' was already added", pPassword);
		return;
	}

	log_info("server", "the password '%s' now can be used to join the server", pPassword);
	pSelf->m_vPasswords.emplace_back(pPassword);
}

void CGameContext::ConRemovePassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pPassword = pResult->GetString(0);

	if(std::find(pSelf->m_vPasswords.begin(), pSelf->m_vPasswords.end(), pPassword) == pSelf->m_vPasswords.end())
	{
		log_warn("server", "the password '%s' is not in the list", pPassword);
		return;
	}

	pSelf->m_vPasswords.erase(
		std::remove(pSelf->m_vPasswords.begin(), pSelf->m_vPasswords.end(), pPassword),
		pSelf->m_vPasswords.end());
	log_warn("server", "removed password '%s'", pPassword);
}

void CGameContext::ConClearPasswords(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	log_info("server", "removing %" PRIzu " password entries", pSelf->m_vPasswords.size());
	pSelf->m_vPasswords.clear();
}

void CGameContext::ConListPasswords(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	log_info("server", "there are %" PRIzu " password entries:", pSelf->m_vPasswords.size());
	int NumPrinted = 0;
	for(std::string &Password : pSelf->m_vPasswords)
	{
		// do not flood the rcon logs if there are a lot of passwords
		if(NumPrinted > 10)
		{
			log_info("server", " and more ...");
			break;
		}
		log_info("server", " '%s'", Password.c_str());
	}
}

void CGameContext::ConAddMapToPool(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Server()->AddMapToRandomPool(pResult->GetString(0));
}

void CGameContext::ConClearMapPool(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Server()->ClearRandomMapPool();
}

void CGameContext::ConRandomMapFromPool(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	const char *pMap = pSelf->Server()->GetRandomMapFromPool();
	if(pMap && pMap[0])
	{
		if(pSelf->m_pController)
		{
			// call change map when in game
			pSelf->m_pController->ChangeMap(pMap);
		}
		else
		{
			// when the controller is not available
			// that means we are currently in a reload
			// or initial server start
			// in that case we just change the sv_map config
			// without triggering a full map change
			// because the controller is nullptr and we would crash
			// the full map change will happen once the controller is initialized
			// https://github.com/ddnet-insta/ddnet-insta/issues/619
			// https://github.com/ddnet/ddnet/issues/11296
			str_copy(g_Config.m_SvMap, pMap);
		}
	}
}

void CGameContext::ConPostStats(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(!pSelf->m_pController->PublishRoundEndStats(false))
	{
		log_warn("ddnet-insta", "no round stats format configured check the README.md and search for round_stats");
	}
}

void CGameContext::ConDeleteRoundStats(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	int Count = 0;
	for(CPlayer *pPlayer : pSelf->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		Count++;
		pPlayer->ResetStats();
	}

	log_info("chatresp", "deleted stats of %d players without saving them!", Count);
}

void CGameContext::ConDeleteSessionStats(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	int Count = 0;
	for(CPlayer *pPlayer : pSelf->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		Count++;
		pPlayer->m_SessionStats.Reset();
	}

	log_info("chatresp", "deleted session stats of %d players", Count);
}

void CGameContext::ConGctfAntibot(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Antibot()->ConsoleCommand("gctf");
}

void CGameContext::ConKnownAntibot(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Antibot()->ConsoleCommand("known");
}

void CGameContext::ConKickEventsAntibot(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pEventIds = pResult->GetString(0);
	if(str_find(pEventIds, "\""))
	{
		log_error("ddnet-insta", "illegal character in event ids");
		return;
	}
	if(str_find(pEventIds, ";"))
	{
		log_error("ddnet-insta", "illegal character in event ids");
		return;
	}

	char aCommand[512];
	str_format(aCommand, sizeof(aCommand), "kick_events %s", pEventIds);
	pSelf->Antibot()->ConsoleCommand(aCommand);
}

void CGameContext::ConDeepJailId(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->DeepJailId(pResult->m_ClientId, pResult->GetVictim(), pResult->GetInteger(1));
}

void CGameContext::ConDeepJailIp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->DeepJailIp(pResult->m_ClientId, pResult->GetString(0), pResult->GetInteger(1));
}

void CGameContext::ConDeepJails(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ListDeepJails(pResult->m_ClientId);
}

void CGameContext::ConUndeepJail(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	const char *pStr = pResult->GetString(0);
	if(str_isallnum(pStr))
	{
		int EntryId = atoi(pStr);
		CIpStorage *pEntry = pSelf->FindIpStorageEntryOfflineAndOnline(EntryId);
		if(!pEntry)
		{
			log_info("deep_jail", "entry id %d not found check undeep_jails for a list", EntryId);
			return;
		}
		pSelf->UndeepJail(pEntry);
		return;
	}

	NETADDR Addr;
	if(net_addr_from_str(&Addr, pStr))
	{
		log_info("deep_jail", "undeep_jail error (invalid network address)");
		return;
	}

	CIpStorage *pEntry = pSelf->m_IpStorageController.FindEntry(&Addr);
	if(!pEntry)
	{
		log_info("deep_jail", "the ip '%s' is not deep jailed check undeep_jails for a full list", pStr);
		return;
	}
	pSelf->UndeepJail(pEntry);
}

void CGameContext::ConInstaPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConPause(pResult, pUserData);
		return;
	}

	if(pSelf->m_pController->GameState() == IGameController::IGS_END_ROUND)
	{
		log_warn("server", "the game can not be paused during round end!");
		return;
	}

	pSelf->m_pController->ToggleGamePause();
}

void CGameContext::ConInstaRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConRestart(pResult, pUserData);
		return;
	}

	const int Seconds = pResult->NumArguments() ? std::clamp(pResult->GetInteger(0), -1, 1000) : 0;
	if(Seconds < 0)
		pSelf->m_pController->AbortWarmup();
	else
		pSelf->m_pController->DoWarmup(Seconds);
}

static bool BlockAccountRconCmd(CGameContext *pSelf, int ClientId, const char *pOperation)
{
	if(!pSelf->m_pController)
	{
		log_error("ddnet-insta", "something went wrong with this rcon command");
		return true;
	}

	// allow admins to reset account passwords even if accounts are off
	// if(!g_Config.m_SvAccounts)
	// {
	// 	log_error("ddnet-insta", "accounts are turned off");
	// 	return true;
	// }

	char aReason[512];
	if(pSelf->m_pController->IsAccountRconCmdRatelimited(ClientId, aReason, sizeof(aReason)))
	{
		log_error("ddnet-insta", "%s failed because of: %s", pOperation, aReason);
		return true;
	}
	return false;
}

void CGameContext::ConAccountList(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	const char *pSearch = "";
	if(pResult->NumArguments())
		pSearch = pResult->GetString(0);

	pSelf->m_pController->RconAccountList(pSearch);
}

void CGameContext::ConAccountForceSetPassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_set_password"))
		return;

	const char *pUsername = pResult->GetString(0);
	const char *pPassword = pResult->GetString(1);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, pPassword, aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconForceSetPassword(pResult->m_ClientId, pUsername, pPassword);
}

void CGameContext::ConAccountForceLogout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_logout"))
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconForceLogout(pResult->m_ClientId, pUsername);
}

void CGameContext::ConLockAccount(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_lock"))
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconLockAccount(pResult->m_ClientId, pUsername);
}

void CGameContext::ConUnlockAccount(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_unlock"))
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconUnlockAccount(pResult->m_ClientId, pUsername);
}

void CGameContext::ConAccountInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_info"))
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconAccountInfo(pResult->m_ClientId, pUsername);
}

void CGameContext::ConAccountStatus(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->RconAccountStatus(pResult->m_ClientId);
}

void CGameContext::ConAddUnclaimableName(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_UnclaimableNames.insert(pResult->GetString(0));
}

void CGameContext::ConRemoveUnclaimableName(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_UnclaimableNames.erase(pResult->GetString(0));
}

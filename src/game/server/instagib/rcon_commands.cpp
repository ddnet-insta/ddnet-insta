#include <base/log.h>
#include <base/system.h>
#include <base/types.h>
#include <base/vmath.h>
#include <engine/antibot.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/ip_storage.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include <game/server/gamecontext.h>

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
	if(!pSelf->m_pController->IsTeamPlay())
		return;

	int Rnd = 0;
	int PlayerTeam = 0;
	int aPlayer[MAX_CLIENTS];

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			aPlayer[PlayerTeam++] = i;

	// pSelf->SendGameMsg(GAMEMSG_TEAM_SHUFFLE, -1);

	//creating random permutation
	for(int i = PlayerTeam; i > 1; i--)
	{
		Rnd = rand() % i;
		int tmp = aPlayer[Rnd];
		aPlayer[Rnd] = aPlayer[i - 1];
		aPlayer[i - 1] = tmp;
	}
	//uneven Number of Players?
	Rnd = PlayerTeam % 2 ? rand() % 2 : 0;

	for(int i = 0; i < PlayerTeam; i++)
		pSelf->m_pController->DoTeamChange(pSelf->m_apPlayers[aPlayer[i]], i < (PlayerTeam + Rnd) / 2 ? TEAM_RED : TEAM_BLUE, false);
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
		pSelf->m_pController->ChangeMap(pMap);
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
	pSelf->ListDeepJails();
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

static bool BlockAccountRconCmd(CGameContext *pSelf, int ClientId, const char *pOperation)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return true;

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

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pSearch = "";
	if(pResult->NumArguments())
		pSearch = pResult->GetString(0);

	pSelf->m_pController->AccountList(pSearch);
}

void CGameContext::ConAccountForceSetPassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_set_password"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pUsername = pResult->GetString(0);
	const char *pPassword = pResult->GetString(1);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, pPassword, aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconForceSetPassword(pPlayer, pUsername, pPassword);
}

void CGameContext::ConAccountForceLogout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_logout"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconForceLogout(pPlayer, pUsername);
}

void CGameContext::ConLockAccount(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_lock"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconLockAccount(pPlayer, pUsername);
}

void CGameContext::ConUnlockAccount(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_unlock"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconUnlockAccount(pPlayer, pUsername);
}

void CGameContext::ConAccountInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountRconCmd(pSelf, pResult->m_ClientId, "acc_info"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pUsername = pResult->GetString(0);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, "placeholder", aBuf, sizeof(aBuf)))
	{
		log_error("ddnet-insta", "%s", aBuf);
		return;
	}

	pSelf->m_pController->RconAccountInfo(pPlayer, pUsername);
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

#include <base/log.h>
#include <base/system.h>
#include <base/types.h>
#include <base/vmath.h>

#include <engine/antibot.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/ip_storage.h>
#include <game/server/player.h>

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

#include <base/system.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include <game/server/gamecontext.h>
#include <string>

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
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "victim with that id not found");
		return;
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' was forced ready by an admin!",
		pSelf->Server()->ClientName(Victim));
	pSelf->SendChat(-1, TEAM_ALL, aBuf);

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

void CGameContext::SwapTeams()
{
	if(!m_pController->IsTeamPlay())
		return;

	SendGameMsg(protocol7::GAMEMSG_TEAM_SWAP, -1);

	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
			m_pController->DoTeamChange(pPlayer, pPlayer->GetTeam() ^ 1, false);
	}

	m_pController->SwapTeamscore();
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

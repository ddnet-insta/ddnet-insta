#include "trainfng.h"

#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CGameControllerTrainFng::CGameControllerTrainFng(CGameContext *pGameServer) :
	CGameControllerSolofng(pGameServer)
{
	m_pGameType = "trainfng";
	m_GameFlags = 0;

	m_SpawnWeapons = ESpawnWeapons::SPAWN_WEAPON_LASER;
	m_DefaultWeapon = WEAPON_LASER;
}

CGameControllerTrainFng::~CGameControllerTrainFng() = default;

void CGameControllerTrainFng::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta TrainFng created by ByFox in 2026",
		"TrainFng was originally created by 35niavlys in 2020",
		"https://github.com/35niavlys/teeworlds-trainfng",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerTrainFng::Tick()
{
	CGameControllerSolofng::Tick();
}

void CGameControllerTrainFng::OnCharacterSpawn(CCharacter *pChr)
{
	CGameControllerSolofng::OnCharacterSpawn(pChr);

	CPlayer *pPlayer = pChr->GetPlayer();
	if(pPlayer->m_pTrainSave)
	{
		pPlayer->m_pTrainSave->Load(pPlayer->GetCharacter());
	}
	else
	{
		SendChatTarget(pPlayer->GetCid(), "You can define your spawn position by writing /setspawn");
	}
}

int CGameControllerTrainFng::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponId)
{
	CGameControllerSolofng::OnCharacterDeath(pVictim, pKiller, WeaponId);

	CPlayer *pPlayer = pVictim->GetPlayer();
	if(!pPlayer)
		return 0;

	const int Team = pVictim->Team();
	if(Team > TEAM_FLOCK && Team < TEAM_SUPER && !Teams().TeamLocked(Team))
	{
		Teams().SetTeamLock(Team, true);
		SendChatTarget(pPlayer->GetCid(), "Your team has been closed");
	}
	return 0;
}

bool CGameControllerTrainFng::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerSolofng::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
	return false;
}

bool CGameControllerTrainFng::CanSelfkill(CPlayer *pPlayer, char *pErrorReason, int ErrorReasonSize)
{
	return true;
}

void CGameControllerTrainFng::OnPauseChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext::ConTogglePause(pResult, pUserData);
}

void CGameControllerTrainFng::OnSpecChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext::ConToggleSpec(pResult, pUserData);
}

bool CGameControllerTrainFng::OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer)
{
	if(CGameControllerSolofng::OnChatMessage(pMsg, Length, Team, pPlayer))
		return true;

	if(pMsg->m_pMessage[0] == '/')
	{
		static const char *s_apIgnoredCommands[] = {
			"/invincible",
			"/hitothers",
			"/infjump",
		};

		for(const char *pCommand : s_apIgnoredCommands)
		{
			if(str_startswith(pMsg->m_pMessage, pCommand))
			{
				SendChatTarget(pPlayer->GetCid(), "This command is disabled for this mode");
				return true;
			}
		}
	}

	return false;
}

REGISTER_GAMEMODE(trainfng, CGameControllerTrainFng(pGameServer));

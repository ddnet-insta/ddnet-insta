#include "ddrace.h"

#include <base/log.h>

#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/insta_core/insta_core.h>

CGameControllerDDRace::CGameControllerDDRace(CGameContext *pGameServer) :
	CGameControllerInstaCore(pGameServer)
{
	m_GameFlags = 0;
	m_pGameType = "ddrace";
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "ddrace";
}

CGameControllerDDRace::~CGameControllerDDRace() = default;

void CGameControllerDDRace::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta flavor of ddnet created in 2026",
		"all the core functionality is provided by ddnet",
		"checkout https://github.com/ddnet/ddnet",
		"or /credits_ddnet for more details",
		"this gametype just has a few ddnet-insta specific extensions",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerDDRace::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaCore::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

bool CGameControllerDDRace::OnTeamChatCmd(IConsole::IResult *pResult)
{
	return CGameControllerInstaCore::OnTeamChatCmd(pResult);
}

REGISTER_GAMEMODE(ddrace, CGameControllerDDRace(pGameServer));

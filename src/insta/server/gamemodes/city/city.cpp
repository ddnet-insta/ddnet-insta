#include "city.h"

#include <generated/insta/mode_account.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

#include <vector>

CGameControllerCity::CGameControllerCity(CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
{
	// if you do not need team red/blue or the red and blue flag from ctf
	// just do m_GameFlags = 0;
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
	m_pGameType = "city";
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "city";
	m_pExtraColumns = nullptr; // new CCityColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
	EnableAccTable(EExtraAccTable::CITY);
	EnableAccTable(EExtraAccTable::MMO);
}

CGameControllerCity::~CGameControllerCity() = default;

// TODO: add methods here, but they should be dynamic

void CGameControllerCity::OnInit(bool ServerStart)
{
	CGameControllerBasePvp::OnInit(ServerStart);
}

void CGameControllerCity::OnCharacterSpawn(CCharacter *pChr)
{
	CGameControllerBasePvp::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

int CGameControllerCity::OnCharacterDeath(CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if(pKiller)
	{
		int Killer = pKiller->GetCid();
		pKiller->m_Account.m_Mode.m_Mmo.m_Stones++;
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "+1 stone you now have %d stones", pKiller->m_Account.m_Mode.m_Mmo.m_Stones);
		SendChatTarget(Killer, aBuf);
	}
	return 0;
}

REGISTER_GAMEMODE(city, CGameControllerCity(pGameServer));

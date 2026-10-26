#include "color_catch.h"

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CGameControllerColorCatch::CGameControllerColorCatch(CGameContext *pGameServer) :
	CGameControllerBaseInstagib(pGameServer)
{
	// if you do not need team red/blue or the red and blue flag from ctf
	// just do m_GameFlags = 0;
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
	m_pGameType = "color_catch";
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "color_catch";
	m_pExtraColumns = nullptr; // new CColorCatchColumns();
	Db()->Stats()->SetExtraColumns(m_pExtraColumns);
	Db()->Stats()->CreateTable(m_pStatsTable);

	// activate additional accounts tables here
	// first define the table in ddnet-insta/datasrc/acc_tables/yourtable.py
	// EnableAccTable(EExtraAccTable::YOUR_TABLE);
}

CGameControllerColorCatch::~CGameControllerColorCatch() = default;

// TODO: add methods here, but they should be dynamic

void CGameControllerColorCatch::OnInit(bool ServerStart)
{
	CGameControllerBaseInstagib::OnInit(ServerStart);
}

void CGameControllerColorCatch::OnCharacterSpawn(CCharacter *pChr)
{
	CGameControllerBaseInstagib::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

int CGameControllerColorCatch::OnCharacterDeath(CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	return 0;
}

REGISTER_GAMEMODE(color_catch, CGameControllerColorCatch(pGameServer));

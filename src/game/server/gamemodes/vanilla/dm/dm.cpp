#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "dm.h"

CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer) :
	CGameControllerVanilla(pGameServer)
{
	m_GameFlags = 0;
	m_pGameType = "DM*";
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "dm";
	m_pExtraColumns = new CDmColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerDM::~CGameControllerDM() = default;

void CGameControllerDM::Tick()
{
	CGameControllerVanilla::Tick();
}

void CGameControllerDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerVanilla::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

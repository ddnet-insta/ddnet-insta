#include "solofng.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CGameControllerSolofng::CGameControllerSolofng(class CGameContext *pGameServer) :
	CGameControllerBaseFng(pGameServer)
{
	m_pGameType = "solofng";
	m_GameFlags = 0;

	m_SpawnWeapons = ESpawnWeapons::SPAWN_WEAPON_LASER;
	m_DefaultWeapon = WEAPON_LASER;

	m_pStatsTable = "solofng";
	m_pExtraColumns = new CSolofngColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerSolofng::~CGameControllerSolofng() = default;

void CGameControllerSolofng::Tick()
{
	CGameControllerBaseFng::Tick();
}

void CGameControllerSolofng::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBaseFng::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}

int CGameControllerSolofng::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerBaseFng::OnCharacterDeath(pVictim, pKiller, WeaponId);
	return 0;
}

bool CGameControllerSolofng::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerBaseFng::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
	return false;
}

void CGameControllerSolofng::Snap(int SnappingClient)
{
	CGameControllerBaseFng::Snap(SnappingClient);
}

REGISTER_GAMEMODE(solofng, CGameControllerSolofng(pGameServer));

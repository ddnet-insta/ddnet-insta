#include "bolofng.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

CGameControllerBolofng::CGameControllerBolofng(class CGameContext *pGameServer) :
	CGameControllerBaseFng(pGameServer)
{
	m_pGameType = "bolofng";
	m_GameFlags = 0;

	m_SpawnWeapons = ESpawnWeapons::SPAWN_WEAPON_GRENADE;
	m_DefaultWeapon = WEAPON_GRENADE;

	m_pStatsTable = "bolofng";
	m_pExtraColumns = new CBolofngColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerBolofng::~CGameControllerBolofng() = default;

void CGameControllerBolofng::Tick()
{
	CGameControllerBaseFng::Tick();
}

void CGameControllerBolofng::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBaseFng::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, g_Config.m_SvGrenadeAmmoRegen ? g_Config.m_SvGrenadeAmmoRegenNum : -1);
}

int CGameControllerBolofng::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerBaseFng::OnCharacterDeath(pVictim, pKiller, WeaponId);
	return 0;
}

bool CGameControllerBolofng::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerBaseFng::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
	return false;
}

void CGameControllerBolofng::Snap(int SnappingClient)
{
	CGameControllerBaseFng::Snap(SnappingClient);
}

REGISTER_GAMEMODE(bolofng, CGameControllerBolofng(pGameServer));

#include "boomfng.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CGameControllerBoomfng::CGameControllerBoomfng(class CGameContext *pGameServer) :
	CGameControllerTeamFng(pGameServer)
{
	m_pGameType = "boomfng";
	m_GameFlags = GAMEFLAG_TEAMS;

	m_SpawnWeapons = ESpawnWeapons::SPAWN_WEAPON_GRENADE;
	m_DefaultWeapon = WEAPON_GRENADE;

	m_pStatsTable = "boomfng";
	m_pExtraColumns = new CBoomfngColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerBoomfng::~CGameControllerBoomfng() = default;

void CGameControllerBoomfng::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBaseFng::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}

REGISTER_GAMEMODE(boomfng, CGameControllerBoomfng(pGameServer));

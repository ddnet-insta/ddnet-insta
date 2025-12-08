#include "fng.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

CGameControllerFng::CGameControllerFng(class CGameContext *pGameServer) :
	CGameControllerTeamFng(pGameServer)
{
	m_pGameType = "fng";
	m_GameFlags = GAMEFLAG_TEAMS;

	m_SpawnWeapons = ESpawnWeapons::SPAWN_WEAPON_LASER;
	m_DefaultWeapon = WEAPON_LASER;

	m_pStatsTable = "fng";
	m_pExtraColumns = new CFngColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerFng::~CGameControllerFng() = default;

void CGameControllerFng::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBaseFng::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}

REGISTER_GAMEMODE(fng, CGameControllerFng(pGameServer));

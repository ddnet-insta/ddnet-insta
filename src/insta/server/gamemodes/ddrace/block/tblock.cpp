#include "tblock.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/ddrace/block/block.h>

CGameControllerTBlock::CGameControllerTBlock(class CGameContext *pGameServer) :
	CGameControllerBlock(pGameServer)
{
	m_pGameType = "tblock";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_DefaultWeapon = WEAPON_GUN;
	m_IsVanillaGameType = false;

	m_pStatsTable = "tblock";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerTBlock::~CGameControllerTBlock() = default;

// Can not use OnKill() here because we need to cover team kills
// https://github.com/ddnet-insta/ddnet-insta/issues/631
int CGameControllerTBlock::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponId)
{
	if(pKiller && WeaponId != WEAPON_GAME)
	{
		// do team scoring
		bool IsTeamOrSelfkill = pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam();
		if(IsTeamOrSelfkill)
			AddTeamscore(pKiller->GetTeam() & 1, -1);
		else
			AddTeamscore(pKiller->GetTeam() & 1, 1);
	}

	// per player scoring and spree tracking
	return CGameControllerBlock::OnCharacterDeath(pVictim, pKiller, WeaponId);
}

REGISTER_GAMEMODE(tblock, CGameControllerTBlock(pGameServer));

#include "foot.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CGameControllerFoot::CGameControllerFoot(class CGameContext *pGameServer) :
	CGameControllerBaseFoot(pGameServer)
{
	m_pGameType = "foot";
	m_GameFlags = GAMEFLAG_TEAMS;

	m_pStatsTable = "foot";
	m_pExtraColumns = new CFootColumns();
	Db()->Stats()->SetExtraColumns(m_pExtraColumns);
	Db()->Stats()->CreateTable(m_pStatsTable);
}

CGameControllerFoot::~CGameControllerFoot() = default;

REGISTER_GAMEMODE(foot, CGameControllerFoot(pGameServer));

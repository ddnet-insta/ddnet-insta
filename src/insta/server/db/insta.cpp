#include "insta.h"

#include <base/log.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/config.h>

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include <insta/server/ddnet_db_utils/ddnet_db_utils.h>
#include <insta/server/extra_columns.h>
#include <insta/server/sql_stats_player.h>

#include <cstdlib>

class IDbConnection;

CGameContext *CDbInsta::GameServer() const { return m_pGameServer; }
IServer *CDbInsta::Server() const { return m_pServer; }

CDbInsta::CDbInsta(CGameContext *pGameServer, CDbConnectionPool *pPool) :
	m_pPool(pPool),
	m_pGameServer(pGameServer),
	m_pServer(pGameServer->Server()),
	m_Stats(pGameServer, pPool, this)
{
}

// this shares one ratelimit with ddnet based requests such as /rank, /times, /top5team and so on
bool CDbInsta::RateLimitPlayer(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(pPlayer == 0)
		return true;
	if(pPlayer->m_LastSqlQuery + (int64_t)g_Config.m_SvSqlQueriesDelay * Server()->TickSpeed() >= Server()->Tick())
		return true;
	pPlayer->m_LastSqlQuery = Server()->Tick();
	return false;
}


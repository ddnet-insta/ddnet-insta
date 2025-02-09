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
	m_pGameServer(pGameServer),
	m_pServer(pGameServer->Server()),
	m_Stats(pGameServer, pPool, this),
	m_Accounts(pGameServer, pPool, this)
{
}

bool CDbInsta::IsRateLimitedPlayer(int ClientId) const
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(pPlayer == 0)
		return true;
	if(pPlayer->m_LastSqlQuery + (int64_t)g_Config.m_SvSqlQueriesDelay * Server()->TickSpeed() >= Server()->Tick())
		return true;
	// if there is a pending logout we should never be able to login
	// again otherwise we could run into a logout ratelimit and get
	// the account into a bad state
	if(pPlayer->m_AccountLogoutQueryResult != nullptr)
		return true;
	return false;
}

// this shares one ratelimit with ddnet based requests such as /rank, /times, /top5team and so on
bool CDbInsta::RateLimitPlayer(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(IsRateLimitedPlayer(ClientId))
		return true;
	pPlayer->m_LastSqlQuery = Server()->Tick();
	return false;
}

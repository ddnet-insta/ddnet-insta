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
	m_Stats(pGameServer, pPool)
{
}

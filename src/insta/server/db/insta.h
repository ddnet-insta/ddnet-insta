#ifndef INSTA_SERVER_DB_INSTA_H
#define INSTA_SERVER_DB_INSTA_H

#include <insta/server/db/accounts.h>
#include <insta/server/db/stats.h>

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;

class CDbInsta
{
	CGameContext *GameServer() const;
	IServer *Server() const;
	CGameContext *m_pGameServer;
	IServer *m_pServer;

	CSqlStats m_Stats;
	CDbAccounts m_Accounts;

public:
	CDbInsta(CGameContext *pGameServer, CDbConnectionPool *pPool);
	~CDbInsta() = default;

	CSqlStats *Stats() { return &m_Stats; }
	CDbAccounts *Accounts() { return &m_Accounts; }

	bool RateLimitPlayer(int ClientId);
	bool IsRateLimitedPlayer(int ClientId) const;
};

#endif

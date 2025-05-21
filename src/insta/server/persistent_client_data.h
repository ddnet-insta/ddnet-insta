#ifndef INSTA_SERVER_PERSISTENT_CLIENT_DATA_H
#define INSTA_SERVER_PERSISTENT_CLIENT_DATA_H

#include <insta/server/account.h>
#include <insta/server/display_name.h>
#include <insta/server/sql_stats_player.h>

class CInstaPersistentClientData
{
public:
	// make sure to read and write the variables you add here in these methods:
	//
	// virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) {};
	// virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) {};

	CSqlStatsPlayer m_SessionStats;
	CDisplayName m_DisplayName;
	CAccount m_Account;
	int m_FirstJoinTime;

	//
	//  Add custom members for mods below this comment to avoid merge conflicts.
	//
};

#endif

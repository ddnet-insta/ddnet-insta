#ifndef INSTA_SERVER_PERSISTENT_CLIENT_DATA_H
#define INSTA_SERVER_PERSISTENT_CLIENT_DATA_H

#include <base/types.h>

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

	// The name of the gametype associated with the controller
	// that performed the persist.
	//
	// This is not supposed to be loaded again this is just to know
	// which controller was active during the persist.
	// Which is interesting to know if the controller changed between
	// persist and load.
	//
	// This can happen when the config sv_gametype changes and a reload is performed.
	// All ddnet-insta based modes that inherit from insta core should be fine.
	// And the all save and load the same data. But if you switch from pure ddnet
	// to a ddnet-insta even just ddrace it will not have set all the ddnet-insta specific
	// fields in the persistet data. So we can not load it.
	//
	// You can also add a new controller that persists specific data
	// and check this string for compatibility when loading the data.
	//
	// If the data was persisted by ddnet and is missing ddnet-insta data
	// the gametype will be a empty string.
	//
	// https://github.com/ddnet-insta/ddnet-insta/issues/669
	// https://github.com/ddnet/ddnet/pull/12250
	char m_aGameType[512] = "";

	NETADDR m_Addr;
	CSqlStatsPlayer m_SessionStats;
	CDisplayName m_DisplayName;
	CAccount m_Account;
	int m_FirstJoinTime;

	//
	//  Add custom members for mods below this comment to avoid merge conflicts.
	//
};

#endif

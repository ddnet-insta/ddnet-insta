#ifndef INSTA_SERVER_PERSISTENT_CLIENT_DATA_H
#define INSTA_SERVER_PERSISTENT_CLIENT_DATA_H

#include <base/types.h>

#include <insta/server/sql_stats_player.h>

// WARNING: member initialization is not working here
//          this is not constructed as a regular C++ object
//          it is a raw malloc() call and then filled in a callback
//          https://github.com/ddnet-insta/ddnet-insta/blob/9f70cfac5fe867ef7de406191bfc4f6f502c27d1/src/engine/server/server.cpp#L3136

class CInstaPersistentClientData
{
public:
	// make sure to read and write the variables you add here in these methods:
	//
	// virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) {};
	// virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) {};

	// when switching from the ddnet gametype to any ddnet-insta gametype
	// we are going to load persistent client data but only the ddnet part
	// is actually set and the ddnet-insta part is uninitialized in that
	// case we need to skip loading it
	// https://github.com/ddnet-insta/ddnet-insta/issues/669
	bool m_IsValid;

	NETADDR m_Addr;
	CSqlStatsPlayer m_SessionStats;

	//
	//  Add custom members for mods below this comment to avoid merge conflicts.
	//
};

#endif

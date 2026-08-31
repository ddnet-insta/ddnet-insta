#ifndef INSTA_SERVER_PERSISTENT_DATA_H
#define INSTA_SERVER_PERSISTENT_DATA_H

#include <insta/server/config_enums.h>

class CInstaPersistentData
{
public:
	// make sure to read and write the variables you add here in these methods:
	//
	// virtual void OnDataPersist(CGameContext::CPersistentData *pData) {}
	// virtual void OnDataRestore(const CGameContext::CPersistentData *pData) {}

	// This has dual use:
	// - It is used to persist the CGameContext::m_aGameType variable across gametype changes
	//   do detectect gametype changes and call the controller hook OnGameTypeChange()
	// - It is used to detect potential incompatibilities between the stored data and the current
	//   controller loading it.
	//   For example the pure ddnet gametype does not persist any ddnet-insta data
	//   so we can not load the uninitialized data when changing gametype from ddnet to a ddnet-insta mode
	//   https://github.com/ddnet-insta/ddnet-insta/issues/669
	char m_aGameType[512] = "";

	CConfigEnums m_ConfigEnums;

	//
	//  Add custom members for mods below this comment to avoid merge conflicts.
	//
};

#endif

#ifndef INSTA_SERVER_PERSISTENT_DATA_H
#define INSTA_SERVER_PERSISTENT_DATA_H

class CInstaPersistentData
{
public:
	// make sure to read and write the variables you add here in these methods:
	//
	// virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) {};
	// virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) {};

	char m_aGameType[512] = "";

	//
	//  Add custom members for mods below this comment to avoid merge conflicts.
	//
};

#endif

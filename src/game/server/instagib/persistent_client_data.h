#ifndef GAME_SERVER_INSTAGIB_PERSISTENT_CLIENT_DATA_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_PERSISTENTCLIENTDATA

#include <game/server/instagib/account.h>

struct CPersistentClientData
{
#endif // IN_CLASS_PERSISTENTCLIENTDATA

	// make sure to read and write the variables you add here in these methods:
	//
	// virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) {};
	// virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) {};

	CAccount m_Account;

	int m_FirstJoinTime;

#ifndef IN_CLASS_PERSISTENTCLIENTDATA
};
#endif

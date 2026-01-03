#ifndef GAME_SERVER_INSTAGIB_PERSISTENT_DATA_H
#define GAME_SERVER_INSTAGIB_PERSISTENT_DATA_H

class CInstaPersistentData
{
public:
	// make sure to read and write the variables you add here in these methods:
	//
	// virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData) {};
	// virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData) {};

	char m_aGameType[512] = "";

#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#undef TRACK_CONFIG_USER_SET
#define TRACK_CONFIG_USER_SET(Name, ScriptName) \
	bool m_UserSet##Name = false; \
	bool m_ModeSet##Name = false;

#include <engine/shared/config_variables_insta.h>

#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR
#undef TRACK_CONFIG_USER_SET

	//
	//  Add custom members for mods below this comment to avoid merge conflicts.
	//
};

#endif

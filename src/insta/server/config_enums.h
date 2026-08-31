#ifndef INSTA_SERVER_CONFIG_ENUMS_H
#define INSTA_SERVER_CONFIG_ENUMS_H

#include <engine/console.h>

class CGameContext;

class CConfigEnums
{
public:
	CConfigEnums();

	void RegisterChains(CGameContext *pGameServer);

#define LINK_CONFIG(ConfigName, ConfigScriptName, EnumName) \
	int ConfigName() const { return m_##ConfigName; }
#include <insta/server/enum_variables.h>
#undef LINK_CONFIG

private:
#define LINK_CONFIG(ConfigName, ConfigScriptName, EnumName) \
	static void Conchain##ConfigName(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData); \
	int m_##ConfigName = 0;
#include <insta/server/enum_variables.h>
#undef LINK_CONFIG
};

#endif

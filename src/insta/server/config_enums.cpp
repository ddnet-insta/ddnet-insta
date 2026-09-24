#include <base/log.h>

#include <engine/shared/console.h>

#include <generated/protocol.h>

#include <game/server/gamecontext.h>

#include <insta/server/enums.h>

CConfigEnums::CConfigEnums()
{
	// all configs are initialized to 0 by default
	// try to use that as your default too if possible
	// only if that cant be avoided set it here
	m_SvBombtagBombWeapon = WEAPON_GRENADE;
}

void CConfigEnums::RegisterChains(CGameContext *pGameServer)
{
#define LINK_CONFIG(ConfigName, ConfigScriptName, EnumName) \
	pGameServer->Console()->Chain(#ConfigScriptName, Conchain##ConfigName, pGameServer);
#include <insta/server/enum_variables.h>
#undef LINK_CONFIG
}

void CConfigEnums::ConchainSvBombtagBombWeapon(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pResult->NumArguments())
	{
		pfnCallback(pResult, pCallbackUserData);
		return;
	}

	int Weapon = 0;
	if(!str_to_weapon(pResult->GetString(0), &Weapon))
	{
		log_error("ddnet-insta", "Error sv_bombtag_bomb_weapon can only be set to one of those values: hammer, gun, shotgun, grenade, laser or ninja");
		return;
	}

	pSelf->m_ConfigEnums.m_SvBombtagBombWeapon = Weapon;
	pfnCallback(pResult, pCallbackUserData);
}

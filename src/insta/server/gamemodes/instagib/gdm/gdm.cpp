#include "gdm.h"

#include <engine/shared/config.h>

#include <game/server/entities/character.h>

CGameControllerGDM::CGameControllerGDM(class CGameContext *pGameServer) :
	CGameControllerInstaBaseDM(pGameServer)
{
	m_pGameType = "gDM";
	m_DefaultWeapon = WEAPON_GRENADE;

	m_pStatsTable = "gdm";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerGDM::~CGameControllerGDM() = default;

void CGameControllerGDM::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta gdm created by +KZ (M0REKZ) in 2024",
		"This is not a ddnet-insta original mode.",
		"The origin and original creator of the gdm gamemode is unknown.",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerGDM::Tick()
{
	CGameControllerInstaBaseDM::Tick();
}

void CGameControllerGDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaBaseDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, g_Config.m_SvGrenadeAmmoRegen ? g_Config.m_SvGrenadeAmmoRegenNum : -1);
}

REGISTER_GAMEMODE(gdm, CGameControllerGDM(pGameServer));

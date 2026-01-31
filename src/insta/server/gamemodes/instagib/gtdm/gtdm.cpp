#include "gtdm.h"

#include <engine/shared/config.h>

#include <game/server/entities/character.h>

CGameControllerGTDM::CGameControllerGTDM(class CGameContext *pGameServer) :
	CGameControllerInstaTDM(pGameServer)
{
	m_pGameType = "gTDM";
	m_DefaultWeapon = WEAPON_GRENADE;

	m_pStatsTable = "gtdm";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerGTDM::~CGameControllerGTDM() = default;

void CGameControllerGTDM::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta gtdm created by +KZ (M0REKZ) in 2024",
		"This is not a ddnet-insta original mode.",
		"The origin and original creator of the gtdm gamemode is unknown.",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerGTDM::Tick()
{
	CGameControllerInstaTDM::Tick();
}

void CGameControllerGTDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaTDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, g_Config.m_SvGrenadeAmmoRegen ? g_Config.m_SvGrenadeAmmoRegenNum : -1);
}

REGISTER_GAMEMODE(gtdm, CGameControllerGTDM(pGameServer));

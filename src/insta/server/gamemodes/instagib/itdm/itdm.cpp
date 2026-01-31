#include "itdm.h"

#include <game/server/entities/character.h>

#include <insta/server/gamemodes/instagib/idm/idm.h>

CGameControllerITDM::CGameControllerITDM(class CGameContext *pGameServer) :
	CGameControllerInstaTDM(pGameServer)
{
	m_pGameType = "iTDM";
	m_DefaultWeapon = WEAPON_LASER;

	m_pStatsTable = "itdm";
	m_pExtraColumns = new CIdmColumns(); // yes itdm and idm have the same db columns
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerITDM::~CGameControllerITDM() = default;

void CGameControllerITDM::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta itdm created by +KZ (M0REKZ) in 2024",
		"This is not a ddnet-insta original mode.",
		"The origin and original creator of the itdm gamemode is unknown.",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerITDM::Tick()
{
	CGameControllerInstaTDM::Tick();
}

void CGameControllerITDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaTDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}

REGISTER_GAMEMODE(itdm, CGameControllerITDM(pGameServer));

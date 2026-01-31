#include "idm.h"

#include <game/server/entities/character.h>

CGameControllerIDM::CGameControllerIDM(class CGameContext *pGameServer) :
	CGameControllerInstaBaseDM(pGameServer)
{
	m_pGameType = "iDM";
	m_DefaultWeapon = WEAPON_LASER;

	m_pStatsTable = "idm";
	m_pExtraColumns = new CIdmColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerIDM::~CGameControllerIDM() = default;

void CGameControllerIDM::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta idm created by +KZ (M0REKZ) in 2024",
		"This is not a ddnet-insta original mode.",
		"The origin and original creator of the idm gamemode is unknown.",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerIDM::Tick()
{
	CGameControllerInstaBaseDM::Tick();
}

void CGameControllerIDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstaBaseDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}

REGISTER_GAMEMODE(idm, CGameControllerIDM(pGameServer));

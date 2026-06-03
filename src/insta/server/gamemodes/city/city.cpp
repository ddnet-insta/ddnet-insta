#include "city.h"

#include <generated/insta/mode_account.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/vanilla/dm/dm.h>

CGameControllerCity::CGameControllerCity(CGameContext *pGameServer) :
	CGameControllerDM(pGameServer)
{
	m_pGameType = "city";
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "city";
	Db()->Stats()->CreateTable(m_pStatsTable);
	EnableAccTable(EExtraAccTable::CITY);
	EnableAccTable(EExtraAccTable::MMO);
}

CGameControllerCity::~CGameControllerCity() = default;

void CGameControllerCity::OnInit(bool ServerStart)
{
	CGameControllerDM::OnInit(ServerStart);
}

bool CGameControllerCity::OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos)
{
	CPlayer *pPlayer = Character.GetPlayer();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "+1 wood you now have %d wood slaps", pPlayer->m_Account.m_Mode.m_City.m_WoodSlap++);
	SendChatTarget(pPlayer->GetCid(), aBuf);
	return false;
}

void CGameControllerCity::OnCharacterSpawn(CCharacter *pChr)
{
	CGameControllerDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

int CGameControllerCity::OnCharacterDeath(CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if(pKiller)
	{
		int Killer = pKiller->GetCid();
		pKiller->m_Account.m_Mode.m_Mmo.m_Stones++;
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "+1 stone you now have %d stones", pKiller->m_Account.m_Mode.m_Mmo.m_Stones);
		SendChatTarget(Killer, aBuf);
	}
	return 0;
}

REGISTER_GAMEMODE(city, CGameControllerCity(pGameServer));

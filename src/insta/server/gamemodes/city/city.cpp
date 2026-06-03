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
	m_pStatsTable = "city";
	Db()->Stats()->CreateTable(m_pStatsTable);
	EnableAccTable(EExtraAccTable::CITY);
}

CGameControllerCity::~CGameControllerCity() = default;

void CGameControllerCity::Tick()
{
	CGameControllerDM::Tick();

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;
		if(!pChr->IsAlive())
			continue;

		// TODO: do not use Tick() use HandleCharacterTiles()
		//       but this depends on https://github.com/ddnet/ddnet/issues/12248

		int MapIndex = GameServer()->Collision()->GetPureMapIndex(pChr->GetPos());
		int TileIndex = GameServer()->Collision()->GetTileIndex(MapIndex);
		int TileFIndex = GameServer()->Collision()->GetFrontTileIndex(MapIndex);

		if(TileIndex == TILE_CITY_CHAIR || TileFIndex == TILE_CITY_CHAIR)
			OnTileChair(pPlayer);
	}
}

void CGameControllerCity::OnTileChair(CPlayer *pPlayer)
{
	if(Server()->Tick() % 50)
		return;

	int ClientId = pPlayer->GetCid();
	if(!pPlayer->m_Account.IsLoggedIn())
	{
		GameServer()->SendBroadcast("You need to /login to farm here", ClientId);
		return;
	}

	int Coins = pPlayer->m_Account.m_Mode.m_City.m_Coins++;
	char aBuf[512];
	const char *pAlignLeft =
		"                                           "
		"                                           "
		"                                           ";
	str_format(aBuf, sizeof(aBuf), "coins: %d (+1)%s", Coins, pAlignLeft);
	GameServer()->SendBroadcast(aBuf, ClientId);
}

void CGameControllerCity::OnCharacterSpawn(CCharacter *pChr)
{
	CGameControllerDM::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

void CGameControllerCity::OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon)
{
	CGameControllerDM::OnKill(pVictim, pKiller, Weapon);

	if(pKiller->m_Account.IsLoggedIn())
	{
		int &Coins = pKiller->m_Account.m_Mode.m_City.m_Coins;
		Coins += 1000;
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "+1000 coins for killing a player (total coins %d)", Coins);
		SendChatTarget(pKiller->GetCid(), aBuf);
	}
}

REGISTER_GAMEMODE(city, CGameControllerCity(pGameServer));

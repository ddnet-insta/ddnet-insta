#ifndef INSTA_SERVER_GAMEMODES_CITY_CITY_H
#define INSTA_SERVER_GAMEMODES_CITY_CITY_H

#include <insta/server/gamemodes/vanilla/dm/dm.h>

enum
{
	TILE_CITY_CHAIR = 160,
};

class CGameControllerCity : public CGameControllerDM
{
public:
	CGameControllerCity(CGameContext *pGameServer);
	~CGameControllerCity() override;

	void Tick() override;
	void OnTileChair(CPlayer *pPlayer);
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon) override;
};
#endif

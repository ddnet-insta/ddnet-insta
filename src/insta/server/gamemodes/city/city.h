#ifndef INSTA_SERVER_GAMEMODES_CITY_CITY_H
#define INSTA_SERVER_GAMEMODES_CITY_CITY_H

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

class CGameControllerCity : public CGameControllerBasePvp
{
public:
	CGameControllerCity(CGameContext *pGameServer);
	~CGameControllerCity() override;

	void OnInit(bool ServerStart) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, CPlayer *pKiller, int Weapon) override;
};
#endif

#ifndef INSTA_SERVER_GAMEMODES_CITY_CITY_H
#define INSTA_SERVER_GAMEMODES_CITY_CITY_H

#include <insta/server/gamemodes/vanilla/dm/dm.h>

class CGameControllerCity : public CGameControllerDM
{
public:
	CGameControllerCity(CGameContext *pGameServer);
	~CGameControllerCity() override;

	void OnInit(bool ServerStart) override;
	bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, CPlayer *pKiller, int Weapon) override;
};
#endif

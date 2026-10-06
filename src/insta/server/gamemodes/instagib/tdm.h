#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_TDM_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_TDM_H

#include <insta/server/gamemodes/instagib/base_instagib.h>

class CGameControllerInstaTDM : public CGameControllerBaseInstagib
{
public:
	CGameControllerInstaTDM(class CGameContext *pGameServer);
	~CGameControllerInstaTDM() override;

	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif

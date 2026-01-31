#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_TDM_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_TDM_H

#include <insta/server/gamemodes/instagib/dm.h>

class CGameControllerInstaTDM : public CGameControllerInstaBaseDM
{
public:
	CGameControllerInstaTDM(class CGameContext *pGameServer);
	~CGameControllerInstaTDM() override;

	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;
};
#endif

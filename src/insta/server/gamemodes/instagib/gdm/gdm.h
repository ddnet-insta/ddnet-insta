#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_GDM_GDM_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_GDM_GDM_H

#include <insta/server/gamemodes/instagib/dm.h>

class CGameControllerGDM : public CGameControllerInstaBaseDM
{
public:
	CGameControllerGDM(class CGameContext *pGameServer);
	~CGameControllerGDM() override;

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
};
#endif

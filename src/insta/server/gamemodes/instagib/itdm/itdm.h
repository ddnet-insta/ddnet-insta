#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_ITDM_ITDM_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_ITDM_ITDM_H

#include "../tdm.h"

class CGameControllerITDM : public CGameControllerInstaTDM
{
public:
	CGameControllerITDM(class CGameContext *pGameServer);
	~CGameControllerITDM() override;

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
};
#endif

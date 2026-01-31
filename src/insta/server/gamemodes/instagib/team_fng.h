#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_TEAM_FNG_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_TEAM_FNG_H

#include "base_fng.h"

class CGameControllerTeamFng : public CGameControllerBaseFng
{
public:
	CGameControllerTeamFng(class CGameContext *pGameServer);
	~CGameControllerTeamFng() override;

	void Tick() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
};
#endif

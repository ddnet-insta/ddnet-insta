#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_BASE_INSTAGIB_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_BASE_INSTAGIB_H

#include "../base_pvp/base_pvp.h"

class CGameControllerInstagib : public CGameControllerBasePvp
{
public:
	CGameControllerInstagib(class CGameContext *pGameServer);
	~CGameControllerInstagib() override;

	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	void OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
};
#endif

#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_BASE_INSTAGIB_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_BASE_INSTAGIB_H

#include "../base_pvp/base_pvp.h"

// Here instagib is not to be confused with the project name ddnet-insta.
// This controller is for gamemodes where weapons kill with one hit
// There is no concept of health, armor or other weapon pickups in those modes.
//
// It is the base for idm, gdm, fng, gtcf, zCatch and so on
class CGameControllerBaseInstagib : public CGameControllerBasePvp
{
public:
	CGameControllerBaseInstagib(class CGameContext *pGameServer);
	~CGameControllerBaseInstagib() override;

	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	void OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
};
#endif

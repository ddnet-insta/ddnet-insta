#ifndef INSTA_SERVER_GAMEMODES_VANILLA_BASE_VANILLA_H
#define INSTA_SERVER_GAMEMODES_VANILLA_BASE_VANILLA_H

#include "../base_pvp/base_pvp.h"

class CGameControllerVanilla : public CGameControllerBasePvp
{
public:
	CGameControllerVanilla(class CGameContext *pGameServer);
	~CGameControllerVanilla() override;

	int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) override;
	bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
};
#endif

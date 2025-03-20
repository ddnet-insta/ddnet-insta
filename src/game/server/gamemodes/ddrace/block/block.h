#ifndef GAME_SERVER_GAMEMODES_DDRACE_BLOCK_BLOCK_H
#define GAME_SERVER_GAMEMODES_DDRACE_BLOCK_BLOCK_H

#include <game/server/gamemodes/base_pvp/base_pvp.h>

class CGameControllerBlock : public CGameControllerPvp
{
public:
	CGameControllerBlock(class CGameContext *pGameServer);
	~CGameControllerBlock() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
};
#endif

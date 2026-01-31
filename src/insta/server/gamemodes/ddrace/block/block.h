#ifndef INSTA_SERVER_GAMEMODES_DDRACE_BLOCK_BLOCK_H
#define INSTA_SERVER_GAMEMODES_DDRACE_BLOCK_BLOCK_H

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

class CGameControllerBlock : public CGameControllerBasePvp
{
public:
	CGameControllerBlock(class CGameContext *pGameServer);
	~CGameControllerBlock() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
	bool SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce) override;
	void OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg) override;
	bool CanSelfkillWhileFrozen(class CPlayer *pPlayer) override { return true; }
	bool HasSuicidePenalty(CPlayer *pPlayer) const override { return false; }
	bool IsDDRaceGameType() const override { return true; }
	bool IsBlockGameType() const override { return true; }
};
#endif

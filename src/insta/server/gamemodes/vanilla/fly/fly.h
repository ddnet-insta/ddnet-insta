#ifndef INSTA_SERVER_GAMEMODES_VANILLA_FLY_FLY_H
#define INSTA_SERVER_GAMEMODES_VANILLA_FLY_FLY_H

#include <insta/server/gamemodes/vanilla/ctf/ctf.h>

class CGameControllerFly : public CGameControllerCTF
{
public:
	CGameControllerFly(class CGameContext *pGameServer);
	~CGameControllerFly() override;

	void Tick() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;

	// TODO: this breaks the config sv_suicide_penalty
	//       should be off by default in this mode instead of always off
	//       see https://github.com/ddnet-insta/ddnet-insta/pull/433
	//       and https://github.com/ddnet-insta/ddnet-insta/issues/308
	bool HasSuicidePenalty(CPlayer *pPlayer) const override { return false; }
};
#endif

#ifndef INSTA_SERVER_GAMEMODES_VANILLA_BASE_CTF_H
#define INSTA_SERVER_GAMEMODES_VANILLA_BASE_CTF_H

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

class CPlayer;

class CGameControllerBaseCTF : public CGameControllerBasePvp
{
public:
	CGameControllerBaseCTF(class CGameContext *pGameServer);
	~CGameControllerBaseCTF() override;

	void Tick() override;
	int OnCharacterDeath(class CCharacter *pVictim, CPlayer *pKiller, int Weapon) override;
	bool CanBeMovedOnBalance(int ClientId) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRoundStats(const CSqlStatsPlayer *pStats, CPlayer *pRequestingPlayer, const char *pRequestedName) override;
};
#endif

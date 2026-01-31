#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_CTF_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_CTF_H

#include <insta/server/gamemodes/instagib/base_instagib.h>

class CGameControllerInstaBaseCTF : public CGameControllerInstagib
{
public:
	CGameControllerInstaBaseCTF(class CGameContext *pGameServer);
	~CGameControllerInstaBaseCTF() override;

	void Tick() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	bool CanBeMovedOnBalance(int ClientId) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
};
#endif

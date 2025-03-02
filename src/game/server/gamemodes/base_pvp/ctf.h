#ifndef GAME_SERVER_GAMEMODES_BASE_PVP_CTF_H
#define GAME_SERVER_GAMEMODES_BASE_PVP_CTF_H

#include "base_pvp.h"

class CGameControllerBaseCTF : public CGameControllerPvp
{
	bool DoWincheckRound() override;

public:
	CGameControllerBaseCTF(class CGameContext *pGameServer);
	~CGameControllerBaseCTF() override;

	void Tick() override;
	void Snap(int SnappingClient) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	bool CanBeMovedOnBalance(int ClientId) override;
	void OnFlagReturn(class CFlag *pFlag) override;
	void OnFlagGrab(class CFlag *pFlag) override;
	void OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;

	// return true to consume the event
	// and supress default ddnet selfkill behavior
	bool OnSelfkill(int ClientId) override;
	bool DropFlag(class CCharacter *pChr) override;
	bool OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId) override;

	void FlagTick();

	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
};
#endif

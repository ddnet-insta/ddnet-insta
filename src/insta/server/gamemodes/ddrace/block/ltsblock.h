#ifndef INSTA_SERVER_GAMEMODES_DDRACE_BLOCK_LTSBLOCK_H
#define INSTA_SERVER_GAMEMODES_DDRACE_BLOCK_LTSBLOCK_H

#include <engine/shared/protocol.h>

#include <insta/server/gamemodes/ddrace/block/block.h>

// Last-team-standing block (ltsblock)
// die -> dead spec, last team standing gets +1 round win
// first team to sv_scorelimit rounds wins the match
// kill/death stats are tracked per player like in regular block
// use sv_freeze_on_spawn to freeze players at the start of each round
class CGameControllerLTSBlock : public CGameControllerBlock
{
public:
	CGameControllerLTSBlock(class CGameContext *pGameServer);
	~CGameControllerLTSBlock() override;

	bool IsDeadSpecGameType() override { return true; }

	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId) override;
	bool DoWincheckRound() override;
	void OnRoundStart() override;
	void YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;
	void YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;

	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;

private:
	// team the player was in when they died, so we can put them back at the start of the next round
	int m_aPreDeathTeam[MAX_CLIENTS];

	// set while we're killing characters to reset the round so OnCharacterDeath doesn't mark the survivors as dead
	bool m_bRoundReset = false;

	int CountAlivePlayersTeam(int Team) const;
	void StartNewRound();
};
#endif

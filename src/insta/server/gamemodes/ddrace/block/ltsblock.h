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

	void Tick() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId) override;
	bool DoWincheckRound() override;
	void OnRoundStart() override;
	void YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;
	void YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;

	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason) override;
	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void OnRoundEnd() override;

private:
	// team the player was in when they died, so we can put them back at the start of the next round
	int m_aPreDeathTeam[MAX_CLIENTS];

	// set while we're killing characters to reset the round so OnCharacterDeath doesn't mark the survivors as dead
	bool m_bRoundReset = false;

	// true while a round is in progress (players have spawned and the fight is on)
	bool m_bRoundActive = false;

	void CountAlivePlayersByTeam(int &AliveRed, int &AliveBlue) const;
	bool IsCharacterFrozen(const CCharacter *pChr) const;
	bool HandleFrozenTeamTimeout(int AliveRed, int AliveBlue);
	void ResetFrozenTeamTimers(); // so stale values do not carry over
	void RestorePlayersFromPreDeathTeam(bool OnlyDeadPlayers);
	void RespawnNonSpectatorPlayers(bool OnlyWithoutCharacter);
	void ResetRoundStateIfEmpty();
	void StartNewRound();

	int m_RedTeamFrozenTicks = 0;
	int m_BlueTeamFrozenTicks = 0;
};
#endif

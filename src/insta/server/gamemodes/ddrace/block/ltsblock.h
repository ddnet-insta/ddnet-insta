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
	~CGameControllerLTSBlock() override = default;

	bool IsDeadSpecGameType() override { return true; }

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;

	void Tick() override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId) override;
	bool DoWincheckRound() override;
	void OnRoundStart() override;
	void YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;
	void YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;

	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason) override;
	void OnRoundEnd() override;

protected:
	// set while we're killing characters to reset the round so OnCharacterDeath doesn't mark the survivors as dead
	bool m_RoundReset = false;

	// true while a round is in progress (players have spawned and the fight is on)
	bool m_RoundActive = false;

	// starts the round start freeze phase: everyone stays frozen until this
	// global tick fires, no matter when they spawned. 0 disables the phase.
	int m_TickToUnFreeze = 0;

	// arms the freeze phase anchored to Server()->Tick() using sv_freeze_on_spawn
	void StartRoundFreezePhase();
	// freezes pChr exactly until m_TickToUnFreeze (or undoes the per respawn
	// freeze when it spawns after the phase already ended)
	void ApplyRoundStartFreeze(CCharacter *pChr);

	void CountAlivePlayersByTeam(int &AliveRed, int &AliveBlue) const;
	bool HandleFrozenTeamTimeout(int AliveRed, int AliveBlue);
	void StartNewRound();

	int m_RedTeamFrozenTicks = 0;
	int m_BlueTeamFrozenTicks = 0;
};
#endif

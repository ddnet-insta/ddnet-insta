#ifndef INSTA_SERVER_GAMEMODES_VANILLA_BOMB_BOMB_H
#define INSTA_SERVER_GAMEMODES_VANILLA_BOMB_BOMB_H

#include <game/server/player.h>

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

#include <random>

#define SQL_COLUMN_FILE <insta/server/gamemodes/vanilla/bomb/sql_columns.h>
#define SQL_COLUMN_CLASS CBombColumns
#include <insta/server/column_template.h>

class CGameControllerBomb : public CGameControllerBasePvp
{
public:
	CGameControllerBomb(class CGameContext *pGameServer);
	~CGameControllerBomb() override;

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void Tick() override;
	void OnReset() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg) override;
	bool DoWincheckRound() override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	bool DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter) override { return false; }
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	void OnRoundEnd() override;
	void YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;
	void YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen) override;
	bool IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage) override;
	bool IsLoser(const CPlayer *pPlayer) override;
	int WinPointsForWin(const CPlayer *pPlayer) override;

	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;

	void SetSkin(class CPlayer *pPlayer);
	void EliminatePlayer(CPlayer *pPlayer, int Weapon);
	void ExplodeBomb(CPlayer *pPlayer, CPlayer *pKiller = nullptr);
	void UpdateTimer();
	void StartBombRound();
	void MakeRandomBomb(int Count);
	void MakeBomb(int ClientId, int Ticks);
	int AmountOfBombs() const;
	void JoinAllPlayers();

	bool IsBombGameType() const override { return true; }
	bool IsDeadSpecGameType() override { return true; }

	bool m_RoundActive = false;

	static std::mt19937 M_S_RANDOM_ENGINE;
};
#endif

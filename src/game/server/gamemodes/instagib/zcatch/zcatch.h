#ifndef GAME_SERVER_GAMEMODES_INSTAGIB_ZCATCH_ZCATCH_H
#define GAME_SERVER_GAMEMODES_INSTAGIB_ZCATCH_ZCATCH_H

#include "../base_instagib.h"

#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats_player.h>

class CZCatchColumns : public CExtraColumns
{
public:
	const char *CreateTable() override
	{
		return
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) sql_name "  " sql_type "  DEFAULT " default ","
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
			;
	}

	const char *SelectColumns() override
	{
		return
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ", " sql_name
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
			;
	}

	const char *InsertColumns() override { return SelectColumns(); }

	const char *UpdateColumns() override
	{
		return
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ", " sql_name " = ? "
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
			;
	}

	const char *InsertValues() override
	{
		return
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ", ?"
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
			;
	}

	void InsertBindings(int *pOffset, IDbConnection *pSqlServer, const CSqlStatsPlayer *pStats) override
	{
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) pSqlServer->Bind##bind_type((*pOffset)++, pStats->m_##name);
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
	}

	void UpdateBindings(int *pOffset, IDbConnection *pSqlServer, const CSqlStatsPlayer *pStats) override
	{
		InsertBindings(pOffset, pSqlServer, pStats);
	}

	void Dump(const CSqlStatsPlayer *pStats, const char *pSystem = "stats") const override
	{
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) \
	dbg_msg(pSystem, "  %s: %d", sql_name, pStats->m_##name);
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
	}

	void MergeStats(CSqlStatsPlayer *pOutputStats, const CSqlStatsPlayer *pNewStats) override
	{
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) \
	pOutputStats->m_##name = Merge##bind_type##merge_method(pOutputStats->m_##name, pNewStats->m_##name);
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
	}

	void ReadAndMergeStats(int *pOffset, IDbConnection *pSqlServer, CSqlStatsPlayer *pOutputStats, const CSqlStatsPlayer *pNewStats) override
	{
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) \
	pOutputStats->m_##name = Merge##bind_type##merge_method(pSqlServer->Get##bind_type((*pOffset)++), pNewStats->m_##name);
#include "sql_columns.h"
#undef MACRO_ADD_COLUMN
	}
};

#define MIN_ZCATCH_PLAYERS 5
#define MIN_ZCATCH_KILLS 4

class CGameControllerZcatch : public CGameControllerInstagib
{
public:
	CGameControllerZcatch(class CGameContext *pGameServer);
	~CGameControllerZcatch() override;

	int m_aBodyColors[MAX_CLIENTS] = {0};

	void KillPlayer(class CPlayer *pVictim, class CPlayer *pKiller, bool KillCounts);
	void OnCaught(class CPlayer *pVictim, class CPlayer *pKiller);
	void ReleasePlayer(class CPlayer *pPlayer, const char *pMsg);

	bool IsZcatchGameType() const override { return true; }
	void Tick() override;
	void Snap(int SnappingClient) override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(class CPlayer *pDisconnectingPlayer, const char *pReason) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	int GetAutoTeam(int NotThisId) override;
	int FreeInGameSlots() override;
	void DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	bool DoWincheckRound() override;
	void OnRoundStart() override;
	void OnRoundEnd() override;
	bool OnSelfkill(int ClientId) override;
	int GetPlayerTeam(class CPlayer *pPlayer, bool Sixup) override;
	bool OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId) override;
	bool IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage) override;
	bool IsLoser(const CPlayer *pPlayer) override;
	bool IsPlaying(const CPlayer *pPlayer) override;
	int WinPointsForWin(const CPlayer *pPlayer) override;
	void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;
	void OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName) override;

	enum class ECatchUpdate
	{
		// called when a player joins the game
		// either on server join
		// or on join from spectators
		CONNECT,
		DISCONNECT,

		// called when a player joins spectators
		SPECTATE,

		// also called when players leave
		// and it becomes a release game
		ROUND_END,
		CAUGHT,
		RELEASE,
	};

	void UpdateCatchTicks(class CPlayer *pPlayer, ECatchUpdate Update);

	enum class ECatchGameState
	{
		// automatic warmup phase if there is less than 5 players
		// will switch to RUNNING as soon as there are enough
		WAITING_FOR_PLAYERS,

		// manually voted release game can also be played with 16 or more players
		// will only change its state to RUNNING if the users vote for it again
		RELEASE_GAME,

		// Regular round is running. Needs 5 or more players to start
		// the amount of points for the win depends on the amount of kills
		//
		// as long as there is still a player that already made a kill
		// and there are enough players in game to end the round the game will keep going
		// otherwise it will revert back to WAITING_FOR_PLAYERS
		RUNNING,
	};
	ECatchGameState m_CatchGameState = ECatchGameState::WAITING_FOR_PLAYERS;
	ECatchGameState CatchGameState() const;
	void SetCatchGameState(ECatchGameState State);
	void ReleaseAllPlayers();

	// sets up the gamestate of a fresh round
	// can happen if the previous round ended
	// or if we switch from a release game to
	// a regular game
	void StartZcatchRound();

	bool CheckChangeGameState();
	bool IsCatchGameRunning() const;

	// colors

	enum class ECatchColors
	{
		TEETIME,
		SAVANDER
	};
	ECatchColors m_CatchColors = ECatchColors::TEETIME;

	// gets the tee's body color based on the amount of its kills
	// the value is the integer that will be sent over the network
	int GetBodyColor(int Kills);

	int GetBodyColorTeetime(int Kills);
	int GetBodyColorSavander(int Kills);

	void SetCatchColors(class CPlayer *pPlayer);
	void OnUpdateZcatchColorConfig() override;

	// returns nullptr if nobody made a kill yet that counts
	CPlayer *PlayerWithMostKillsThatCount();
};
#endif // GAME_SERVER_GAMEMODES_ZCATCH_H

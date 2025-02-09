#ifndef GAME_SERVER_INSTAGIB_SQL_STATS_H
#define GAME_SERVER_INSTAGIB_SQL_STATS_H

#include <engine/server/databases/connection_pool.h>
#include <engine/shared/protocol.h>
#include <game/server/instagib/account.h>
#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/scoreworker.h>

#include "sql_accounts.h"

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;

enum class EInstaSqlRequestType
{
	// prints direct messages
	DIRECT,

	// prints chat all messages
	ALL,

	// prints broadcast
	BROADCAST,

	// /statsall chat command
	CHAT_CMD_STATSALL,

	// /rank_xxx chat command
	CHAT_CMD_RANK,

	// /multis chat command
	CHAT_CMD_MULTIS,

	// /steals chat command
	CHAT_CMD_STEALS,

	// initial stats load on connect
	PLAYER_DATA,
};

struct CInstaSqlResult : ISqlResult
{
	CInstaSqlResult();

	enum
	{
		MAX_MESSAGES = 10,
	};

	EInstaSqlRequestType m_MessageKind;

	char m_aaMessages[MAX_MESSAGES][512];
	char m_aBroadcast[1024];
	struct
	{
		char m_aRequestedPlayer[MAX_NAME_LENGTH];
	} m_Info = {};

	CSqlStatsPlayer m_Stats;

	// if it is a /rank_* request the resulting rank will be in m_Rank
	int m_Rank = 0;

	// and the m_RankedScore is the amount it was ranked by
	// so this might be the amount of kills for /rank_kills
	int m_RankedScore = 0;

	// shown to the user
	char m_aRankColumnDisplay[128];

	// used as a sql table column
	char m_aRankColumnSql[128];

	void SetVariant(EInstaSqlRequestType RequestType);
};

struct CSqlInstaData : ISqlData
{
	CSqlInstaData(std::shared_ptr<ISqlResult> pResult) :
		ISqlData(std::move(pResult))
	{
	}

	~CSqlInstaData() override;

	int m_DebugStats = 0;
	CExtraColumns *m_pExtraColumns = nullptr;
};

// read request
struct CSqlPlayerStatsRequest : CSqlInstaData
{
	CSqlPlayerStatsRequest(std::shared_ptr<CInstaSqlResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}
	EInstaSqlRequestType m_RequestType = EInstaSqlRequestType::DIRECT;

	// object being requested, player (16 bytes)
	char m_aName[MAX_NAME_LENGTH];
	char m_aRequestingPlayer[MAX_NAME_LENGTH];
	// relevant for /top5 kind of requests
	int m_Offset;

	// shown to the user
	char m_aRankColumnDisplay[128];

	// used as a sql table column
	char m_aRankColumnSql[128];

	// table name depends on gametype
	char m_aTable[128];

	// SQL ASC or DESC
	char m_aOrderBy[128];
};

// read request
struct CSqlPlayerAccountRequest : CSqlInstaData
{
	CSqlPlayerAccountRequest(std::shared_ptr<CAccountPlayerResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}
	EAccountPlayerRequestType m_RequestType = EAccountPlayerRequestType::DIRECT;

	// warning do not access anything from the main thread
	// using m_ClientId
	// this should only be used for logging
	// the player might already by disconnected when
	// the thread pool picks it up
	int m_ClientId;
	char m_aUsername[MAX_NAME_LENGTH];
	char m_aOldPassword[MAX_NAME_LENGTH];
	char m_aNewPassword[MAX_NAME_LENGTH];
	char m_aTimestamp[TIMESTAMP_STR_LENGTH];

	char m_aServerIp[64];
	int m_ServerPort;
	char m_aUserIpAddr[64];
};

// data to be writtem
struct CSqlPlayerAccountData : CSqlInstaData
{
	CSqlPlayerAccountData(std::shared_ptr<CAccountManagementResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	CAccount m_Account;
};

// data to be writtem
struct CSqlPlayerAccountRconCmdData : CSqlInstaData
{
	CSqlPlayerAccountRconCmdData(std::shared_ptr<CAccountRconCmdResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	EAccountRconPlayerRequestType m_RequestType = EAccountRconPlayerRequestType::LOG_INFO;

	// name of the admin that ran the rcon
	// command that triggerd the request
	char m_aAdminName[MAX_NAME_LENGTH];

	char m_aUsername[MAX_NAME_LENGTH];
	char m_aPassword[MAX_NAME_LENGTH];

	char m_aServerIp[64];
	int m_ServerPort;
};

// data to be writtem
struct CSqlPlayerFastcapData : CSqlInstaData
{
	CSqlPlayerFastcapData(std::shared_ptr<CInstaSqlResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	float m_Time;
	char m_aTimestamp[TIMESTAMP_STR_LENGTH];

	char m_aGameUuid[UUID_MAXSTRSIZE];

	// object being requested, player (16 bytes)
	char m_aName[MAX_NAME_LENGTH];
	char m_aRequestingPlayer[MAX_NAME_LENGTH];

	char m_aGametype[128];
	char m_aMap[128];
	bool m_Grenade = false;
	bool m_StatTrack = false;
};

// read request
struct CSqlPlayerFastcapRequest : CSqlInstaData
{
	CSqlPlayerFastcapRequest(std::shared_ptr<CInstaSqlResult> pResult, int DebugStats) :
		CSqlInstaData(std::move(pResult))
	{
		m_DebugStats = DebugStats;
	}

	// object being requested, player (16 bytes)
	char m_aName[MAX_NAME_LENGTH];
	char m_aRequestingPlayer[MAX_NAME_LENGTH];
	// relevant for /top5 kind of requests
	int m_Offset;

	char m_aGametype[128];
	char m_aMap[128];
	bool m_Grenade = false;
	bool m_OnlyStatTrack = false;
};

// data to be written
struct CSqlSaveRoundStatsData : CSqlInstaData
{
	CSqlSaveRoundStatsData(int DebugStats) :
		CSqlInstaData(nullptr)
	{
		m_DebugStats = DebugStats;
	}

	char m_aName[128];
	char m_aTable[128];
	CSqlStatsPlayer m_Stats;
};

struct CSqlLogoutAllRequest : CSqlInstaData
{
	CSqlLogoutAllRequest(int DebugStats) :
		CSqlInstaData(nullptr)
	{
		m_DebugStats = DebugStats;
	}
	char m_aServerIp[128];
	int m_ServerPort;
};

struct CSqlCreateTableRequest : ISqlData
{
	CSqlCreateTableRequest() :
		ISqlData(nullptr)
	{
	}
	char m_aName[128];
	char m_aColumns[2048];
};

enum class ESqlBackend
{
	SQLITE3,
	MYSQL
};

class CSqlStats
{
	CDbConnectionPool *m_pPool;
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
	CGameContext *m_pGameServer;
	IServer *m_pServer;

	CExtraColumns *m_pExtraColumns = nullptr;

	static bool AddIntColumn(IDbConnection *pSqlServer, const char *pTableName, const char *pColumnName, int Default, char *pError, int ErrorSize);

	static bool AddColumnIntDefault0Sqlite3(IDbConnection *pSqlServer, const char *pTableName, const char *pColumnName, char *pError, int ErrorSize);
	static bool AddColumnIntDefault0Mysql(IDbConnection *pSqlServer, const char *pTableName, const char *pColumnName, char *pError, int ErrorSize);

	// non ratelimited server side queries
	static bool CreateTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool CreateFastcapTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool SaveRoundStatsThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool LogoutAllAccountsOnCurrentServerThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

	// ratelimited user queries

	static bool ShowStatsWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowFastcapRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ShowFastcapTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

	static bool SaveFastcapWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

	std::shared_ptr<CInstaSqlResult> NewInstaSqlResult(int ClientId);
	std::shared_ptr<CAccountPlayerResult> NewInstaAccountResult(int ClientId);

	// Creates for player database requests
	void ExecPlayerStatsThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pName,
		const char *pTable,
		EInstaSqlRequestType RequestType);

	void ExecPlayerRankOrTopThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pName,
		const char *pRankColumnDisplay,
		const char *pRankColumnSql,
		const char *pTable,
		const char *pOrderBy,
		int Offset);

	void ExecPlayerFastcapRankOrTopThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pName,
		const char *pMap,
		const char *pGametype,
		bool Grenade,
		bool OnlyStatTrack,
		int Offset);

	// TODO: make sure this is not used for saving because it can be ratelimited
	//
	// should be used for register and login and not for logout
	void ExecPlayerAccountThread(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, Write w, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pUsername,
		const char *pOldPassword,
		const char *pNewPassword,
		EAccountPlayerRequestType RequestType);

	bool RateLimitPlayer(int ClientId);

public:
	CSqlStats(CGameContext *pGameServer, CDbConnectionPool *pPool);
	~CSqlStats() = default;

	bool IsRateLimitedPlayer(int ClientId) const;

	// hack to avoid editing connection.h in ddnet code
	static ESqlBackend DetectBackend(IDbConnection *pSqlServer);

	void SetExtraColumns(CExtraColumns *pExtraColumns);

	void CreateTable(const char *pName);
	void CreateFastcapTable();
	void CreateAccountsTable();
	void SaveRoundStats(const char *pName, const char *pTable, CSqlStatsPlayer *pStats);
	void SaveFastcap(int ClientId, int TimeTicks, const char *pTimestamp, bool Grenade, bool StatTrack);

	void LoadInstaPlayerData(int ClientId, const char *pTable);

	void ShowStats(int ClientId, const char *pName, const char *pTable, EInstaSqlRequestType RequestType);
	void ShowRank(int ClientId, const char *pName, const char *pRankColumnDisplay, const char *pRankColumnSql, const char *pTable, const char *pOrderBy);
	void ShowTop(int ClientId, const char *pName, const char *pRankColumnDisplay, const char *pRankColumnSql, const char *pTable, const char *pOrderBy, int Offset);
	void ShowFastcapRank(int ClientId, const char *pName, const char *pMap, const char *pGametype, bool Grenade, bool OnlyStatTrack);
	void ShowFastcapTop(int ClientId, const char *pName, const char *pMap, const char *pGametype, bool Grenade, bool OnlyStatTrack, int Offset);

	// TODO: horrible name should register and login really be shared?
	//       should the argument be a union struct?
	// ratelimited per player account requests
	void Account(int ClientId, const char *pUsername, const char *pOldPassword, const char *pNewPassword, EAccountPlayerRequestType RequestType);

	// for now only used for resetting passwords
	// can in the future also be used to
	// - freeze
	// - unfreeze
	// - force logout if stuck
	// - dump account info
	void AccountRconCmd(int ClientId, const char *pUsername, const char *pPassword, EAccountRconPlayerRequestType RequestType);

	// unratelimited management request
	void SaveAndLogoutAccount(class CPlayer *pPlayer, const char *pSuccessMessage);

	// performs only one sql query to set all accounts to logged out in the db
	// does not actually operate on in game player instances
	// you still have to logout all CPlayer instances
	void LogoutAllAccountsOnCurrentServer();
};

#endif

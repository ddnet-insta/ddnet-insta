#ifndef INSTA_SERVER_DB_ACCOUNTS_WORKER_ACCOUNTS_WORKER_H
#define INSTA_SERVER_DB_ACCOUNTS_WORKER_ACCOUNTS_WORKER_H

#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/protocol.h>

#include <generated/insta/mode_account.h>

#include <game/server/scoreworker.h>

#include <insta/server/account.h>
#include <insta/server/display_name.h>
#include <insta/server/extra_columns.h>
#include <insta/server/sql_stats_player.h>

#include <cstdint>

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;
class CDbInsta;

enum class EAccountChatCmd
{
	// prints direct messages
	DIRECT,

	// prints chat all messages
	ALL,

	// prints to log (and rcon console depending on config)
	LOG_INFO,

	// prints to log (and rcon console depending on config)
	LOG_ERROR,

	// prints broadcast
	BROADCAST,

	// wrong password for example
	LOGIN_FAILED,

	// /register chat command
	CHAT_CMD_REGISTER,

	// /login chat command
	CHAT_CMD_LOGIN,

	// /changepassword chat command
	CHAT_CMD_CHANGE_PASSWORD,

	// /displayname chat command
	CHAT_CMD_DISPLAY_NAME,

	// /lockname chat command
	CHAT_CMD_LOCK_NAME,

	// this is used for debugging only
	// /slow_account_operation chat command
	CHAT_CMD_SLOW_ACCOUNT_OPERATION,
};

// TODO: can this be split into two enums?
//       the request and result values are not shared at the moment
//       so there are two switch statements where half of the enum is not covered
enum class EAccountRconCmd
{
	// prints direct messages
	DIRECT,

	// prints chat all messages
	ALL,

	// prints to log (and rcon console depending on config)
	LOG_INFO,

	// prints to log (and rcon console depending on config)
	LOG_ERROR,

	// acc_set_password rcon command
	ACC_SET_PASSWORD,

	// acc_logout rcon command
	ACC_LOGOUT,

	// acc_lock rcon command
	ACC_LOCK,

	// acc_unlock rcon command
	ACC_UNLOCK,

	// acc_info rcon command
	ACC_INFO,
};

// player bound account requests
// ratelimited for every player
// the query is guaranteed to finish
// but the result is only processed in the main
// thread if the player stays connected until the end
struct CAccountPlayerResult : ISqlResult
{
	CAccountPlayerResult();

	enum
	{
		MAX_MESSAGES = 10,
	};

	EAccountChatCmd m_MessageKind;

	union
	{
		char m_aaMessages[MAX_MESSAGES][512];
		char m_aBroadcast[1024];
		CAccount m_Account;
		struct
		{
			char m_aNameOwner[MAX_USERNAME_LENGTH];
			char m_aDisplayName[MAX_NAME_LENGTH];
			bool m_IsProtected;
		} m_NameClaim;
		struct
		{
			char m_aError[512];
			char m_aUsername[MAX_USERNAME_LENGTH];
		} m_LoginFailed;
	} m_Data = {};

	void SetVariant(EAccountChatCmd RequestType);
	void SetLoginFailed(const char *pUsername, const char *pError);
};

// this is only used for logout for now
// which does not return any values
struct CAccountManagementResult : ISqlResult
{
	CAccountManagementResult(const char *pSuccessMessage);
	char m_aMessage[512];
};

struct CAccountRconCmdResult : ISqlResult
{
	CAccountRconCmdResult(uint32_t UniqueClientId);

	enum
	{
		MAX_MESSAGES = 10,
	};
	char m_aaMessages[MAX_MESSAGES][512];
	EAccountRconCmd m_MessageKind = EAccountRconCmd::LOG_INFO;

	// admin that ran the rcon command
	// not a regular client id but a unique id
	uint32_t m_UniqueClientId = 0;

	// only initialized for ACC_INFO kind
	CAccount m_Account;

	// only initialized for ACC_INFO kind
	char m_aUsername[MAX_USERNAME_LENGTH];
};

struct CSqlAccData : ISqlData
{
	CSqlAccData(std::shared_ptr<ISqlResult> pResult) :
		ISqlData(std::move(pResult))
	{
	}

	~CSqlAccData() override = default;

	int m_DebugAccounts = 0;
};

struct CSqlLogoutAllRequest : ISqlData
{
	CSqlLogoutAllRequest() :
		ISqlData(nullptr)
	{
	}
	char m_aServerIp[128];
	int m_ServerPort;
};

struct CSqlCreateExtraAccountsTablesRequest : ISqlData
{
	CSqlCreateExtraAccountsTablesRequest() :
		ISqlData(nullptr)
	{
	}
	std::vector<EExtraAccTable> m_vTables;
};

// read request
struct CSqlPlayerAccountRequest : CSqlAccData
{
	CSqlPlayerAccountRequest(std::shared_ptr<CAccountPlayerResult> pResult, int DebugAccounts) :
		CSqlAccData(std::move(pResult))
	{
		m_DebugAccounts = DebugAccounts;
	}
	EAccountChatCmd m_RequestType = EAccountChatCmd::DIRECT;

	// warning do not access anything from the main thread
	// using m_ClientId
	// this should only be used for logging
	// the player might already by disconnected when
	// the thread pool picks it up
	int m_ClientId;
	char m_aUsername[MAX_NAME_LENGTH];
	char m_aDisplayName[MAX_NAME_LENGTH];
	char m_aOldPassword[MAX_NAME_LENGTH];
	char m_aNewPassword[MAX_NAME_LENGTH];
	char m_aTimestamp[TIMESTAMP_STR_LENGTH];

	char m_aServerIp[64];
	int m_ServerPort;
	char m_aUserIpAddr[64];

	std::vector<EExtraAccTable> m_vTables;
};

// data to be writtem
struct CSqlPlayerAccountRconCmdData : CSqlAccData
{
	CSqlPlayerAccountRconCmdData(std::shared_ptr<CAccountRconCmdResult> pResult, int DebugAccounts) :
		CSqlAccData(std::move(pResult))
	{
		m_DebugAccounts = DebugAccounts;
	}

	EAccountRconCmd m_RequestType = EAccountRconCmd::LOG_INFO;

	// name of the admin that ran the rcon
	// command that triggered the request
	char m_aAdminName[MAX_NAME_LENGTH];

	char m_aUsername[MAX_NAME_LENGTH];
	char m_aPassword[MAX_NAME_LENGTH];

	char m_aServerIp[64];
	int m_ServerPort;
};

// data to be writtem
struct CSqlPlayerAccountData : CSqlAccData
{
	CSqlPlayerAccountData(std::shared_ptr<CAccountManagementResult> pResult, int DebugAccounts) :
		CSqlAccData(std::move(pResult))
	{
		m_DebugAccounts = DebugAccounts;
	}

	CAccount m_Account;
	std::vector<EExtraAccTable> m_vTables;
};

struct CCheckNameClaimResult : ISqlResult
{
	CDisplayNameOwner m_Owner;
};

// read request
struct CSqlCheckNameClaimRequest : ISqlData
{
	CSqlCheckNameClaimRequest(std::shared_ptr<ISqlResult> pResult) :
		ISqlData(std::move(pResult))
	{
	}

	// in game display name
	char m_aDisplayName[MAX_NAME_LENGTH];
};

class CAccountsWorker
{
public:
	static bool CreateAccountsTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool CreateExtraAccountsTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool ChatCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool RconCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool AccountSaveAndLogoutWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool LogoutAllAccountsOnCurrentServerThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool CheckNameClaimedWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

private:
	// chat_cmds.cpp
	static bool ChatCmdLogin(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize);
	static bool ChatCmdRegister(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize);
	static bool ChatCmdChangePassword(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ChatCmdDisplayName(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ChatCmdLockName(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool ChatCmdSlowOperation(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize);

	// TODO: use this a bit more as return type instead of bool
	//       for example for AccountInfo()
	enum class EResult
	{
		// the sql operation finished successfully
		// and the user input was valid
		SUCCESS,

		// this means "not found" or "not updated"
		// there was no serious error but the user input
		// was probably invalid
		//
		// this is the general expected unhappy path
		INVALID,

		// there was an unexpected fatal database error
		// even funny user input should never trigger this
		// this means the database schema is corrupted
		// or the database connection died
		// or some other serious issue which needs developer or admin attention
		FATAL_ERROR,
	};

	// returns false on fatal db error
	// and true in all other cases
	static bool SetPassword(IDbConnection *pSqlServer, const char *pUsername, const char *pPassword, char *pError, int ErrorSize);

	// returns false on fatal db error
	// and true in all other cases
	static bool ForceLogout(IDbConnection *pSqlServer, const struct CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);

	// returns false on fatal db error
	// and true in all other cases
	static bool LockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);

	// returns false on fatal db error
	// and true in all other cases
	static bool UnlockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);

	// returns false on fatal db error
	// and true in all other cases
	static bool AccountInfo(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);

	// writes account details to pAccount if it returned SUCCESS
	//
	// you can pass nullptr for pAccount if you do not need the details
	//
	// vTables specifies which additional tables should be loaded
	static EResult LoadAccount(IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize);

	// returns false on error
	// sets pColumn to Value where the username is pUsername
	//
	// WARNING: pColumn should never be user input! Otherwise there will be sql injections!
	//
	// will fail with an fatal error if pUsername is not found that should be checked first
	static bool SetAccountInt(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, int Value, char *pError, int ErrorSize);

	// returns false on error
	// sets pColumn to pValue where the username is pUsername
	//
	// WARNING: pColumn should never be user input! Otherwise there will be sql injections!
	//
	// will fail with an fatal error if pUsername is not found that should be checked first
	static bool SetAccountString(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, const char *pValue, char *pError, int ErrorSize);

	// returns false on fatal db error
	// returns true on success which can be a found claimed name or not
	//
	// if an account claimed pDisplayName it will write the accounts username into pOwner
	//
	// pDisplayName - input nick name to check if it is claimed
	// pOwner - the output object where the owner will be written to
	static bool GetDisplayNameOwner(IDbConnection *pSqlServer, const char *pDisplayName, CDisplayNameOwner *pOwner, char *pError, int ErrorSize);
};

#endif

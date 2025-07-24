#ifndef GAME_SERVER_INSTAGIB_SQL_ACCOUNTS_H
#define GAME_SERVER_INSTAGIB_SQL_ACCOUNTS_H

#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/protocol.h>
#include <game/server/instagib/account.h>
#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/scoreworker.h>

#include <cstdint>

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;

// TODO: rename to EAccountChatCmd
enum class EAccountPlayerRequestType
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

	// /claimname chat command
	CHAT_CMD_CLAIM_NAME,

	// this is used for debugging only
	// /slow_account_operation chat command
	CHAT_CMD_SLOW_ACCOUNT_OPERATION,
};

// TODO: rename to EAccountRconCmd
// TODO: can this be split into two enums?
//       the request and result values are not shared at the moment
//       so there are two switch statements where half of the enum is not covered
enum class EAccountRconPlayerRequestType
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
// the query is guranteed to finish
// but the result is only processed in the main
// thread if the player stays connected until the end
struct CAccountPlayerResult : ISqlResult
{
	CAccountPlayerResult();

	enum
	{
		MAX_MESSAGES = 10,
	};

	EAccountPlayerRequestType m_MessageKind;

	union
	{
		char m_aaMessages[MAX_MESSAGES][512];
		char m_aBroadcast[1024];
		CAccount m_Account;
		struct
		{
			char m_aNameOwner[MAX_USERNAME_LENGTH];
			char m_aDisplayName[MAX_NAME_LENGTH];
		} m_NameClaim = {};
	} m_Data = {};

	void SetVariant(EAccountPlayerRequestType RequestType);
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
	CAccountRconCmdResult(int UniqueClientId);

	enum
	{
		MAX_MESSAGES = 10,
	};
	char m_aaMessages[MAX_MESSAGES][512];
	EAccountRconPlayerRequestType m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;

	// admin that ran the rcon command
	// not a regular client id but a unique id
	uint32_t m_UniqueClientId = 0;

	// only initialized for ACC_INFO kind
	CAccount m_Account;

	// only initialized for ACC_INFO kind
	char m_aUsername[MAX_USERNAME_LENGTH];
};

class CSqlAccounts
{
public:
	static bool AccountWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool AccountSaveAndLogoutWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool AccountRconCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool CreateAccountsTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

private:
	// TODO: fix the messy mix of true being used for error and success
	//       this should probably be fixed upstream since the db code is violating the
	//       CONTRIBUTING.md which states that "true" means success
	//       Maybe a bool is also the wrong thing here because there are 3 states not 2:
	//       - success
	//       - fatal sql error
	//       - logic error or wrong user input like "username not found"

	// returns false on fatal db error
	// and true in all other cases
	static bool ChangePasswordRequest(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

	// returns false on fatal db error
	// and true in all other cases
	static bool ClaimNameRequest(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

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

	// returns false on error
	// writes account details to pAccount if it returned true
	//
	// you can pass nullptr for pAccount if you do not need the details
	static bool LoadAccount(IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, char *pError, int ErrorSize);

	// returns false on error
	// sets pColumn to Value where the username is pUsername
	//
	// WARNING pColumn should never be user input! Otherwise there will be sql injections!
	//
	// will fail with an fatal error if pUsername is not found that should be checked first
	static bool SetAccountInt(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, int Value, char *pError, int ErrorSize);

	// returns false on error
	// sets pColumn to pValue where the username is pUsername
	//
	// WARNING pColumn should never be user input! Otherwise there will be sql injections!
	//
	// will fail with an fatal error if pUsername is not found that should be checked first
	static bool SetAccountString(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, const char *pValue, char *pError, int ErrorSize);

public:
	// returns false on fatal db error
	// returns true on success which can be a found claimed name or not
	//
	// if an account claimed pDisplayName it will write the accounts username into pUsername
	//
	// pDisplayName - input nick name to check if it is claimed
	// pUsername - output buffer where the account owner will be written to (can be nullptr if not needed)
	// UsernameSize - size of the pUsername buffer
	static bool GetDisplayNameOwnerUsername(IDbConnection *pSqlServer, const char *pDisplayName, char *pUsername, int UsernameSize, char *pError, int ErrorSize);
};

#endif

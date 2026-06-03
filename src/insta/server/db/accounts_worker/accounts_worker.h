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

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;
class CDbInsta;

// this is only used for logout for now
// which does not return any values
struct CAccountManagementResult : ISqlResult
{
	CAccountManagementResult(const char *pSuccessMessage);
	char m_aMessage[512];
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
	static bool AccountSaveAndLogoutWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool LogoutAllAccountsOnCurrentServerThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);
	static bool CheckNameClaimedWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

protected:
	// This type should be used instead of bool for helpers
	// that can result in an unhappy path without a critical database error
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

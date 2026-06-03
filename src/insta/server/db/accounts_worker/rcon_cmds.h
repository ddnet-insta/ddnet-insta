#ifndef INSTA_SERVER_DB_ACCOUNTS_WORKER_RCON_CMDS_H
#define INSTA_SERVER_DB_ACCOUNTS_WORKER_RCON_CMDS_H

#include <engine/shared/protocol.h>

#include <insta/server/db/accounts_worker/accounts_worker.h>

struct ISqlData;
class IDbConnection;

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

	// acc_displayname rcon command
	ACC_DISPLAYNAME,
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

	// only initialized for ACC_INFO and ACC_DISPLAYNAME kind
	char m_aUsername[MAX_USERNAME_LENGTH] = "";

	// only set for ACC_DISPLAYNAME
	char m_aDisplayname[MAX_NAME_LENGTH] = "";
	// only set for ACC_DISPLAYNAME
	bool m_GotDeleted = false;
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

class CRconCmdsWorker : public CAccountsWorker
{
public:
	static bool RconCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

private:
	static bool CmdSetPassword(IDbConnection *pSqlServer, const struct CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);
	static bool CmdForceLogout(IDbConnection *pSqlServer, const struct CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);
	static bool CmdLockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);
	static bool CmdUnlockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);
	static bool CmdAccountInfo(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);
	static bool CmdAccountDisplayname(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize);
};

#endif

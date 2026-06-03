#ifndef INSTA_SERVER_DB_ACCOUNTS_WORKER_CHAT_CMDS_H
#define INSTA_SERVER_DB_ACCOUNTS_WORKER_CHAT_CMDS_H

#include <insta/server/db/accounts_worker/accounts_worker.h>

struct ISqlData;
class IDbConnection;

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

class CChatCmdsWorker : public CAccountsWorker
{
public:
	static bool ChatCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize);

private:
	static bool CmdLogin(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize);
	static bool CmdRegister(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize);
	static bool CmdChangePassword(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool CmdDisplayName(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool CmdLockName(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool CmdSlowOperation(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize);
};

#endif

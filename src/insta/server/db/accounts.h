#ifndef INSTA_SERVER_DB_ACCOUNTS_H
#define INSTA_SERVER_DB_ACCOUNTS_H

#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/protocol.h>

#include <insta/server/account.h>
#include <insta/server/db/accounts_worker/accounts_worker.h>
#include <insta/server/extra_columns.h>
#include <insta/server/sql_stats_player.h>

#include <optional>
#include <vector>

struct ISqlData;
class IDbConnection;
class IServer;
class CGameContext;
class CDbInsta;

class CDbAccounts
{
	CDbConnectionPool *m_pPool = nullptr;
	CGameContext *m_pGameServer = nullptr;
	IServer *m_pServer = nullptr;
	CDbInsta *m_pInstaDatabase = nullptr;
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }
	CDbInsta *Db() { return m_pInstaDatabase; }

	std::shared_ptr<CAccountPlayerResult> NewPlayerResult(int ClientId);

	// WARNING: make sure this is not used for saving because it can be ratelimited
	//
	// should be used for register and login and not for logout
	void ExecPlayerThreadRatelimited(
		bool (*pFuncPtr)(IDbConnection *, const ISqlData *, Write w, char *pError, int ErrorSize),
		const char *pThreadName,
		int ClientId,
		const char *pUsername,
		const char *pDisplayName,
		const char *pOldPassword,
		const char *pNewPassword,
		CAccountChatCmd RequestType);

public:
	CDbAccounts(CGameContext *pGameServer, CDbConnectionPool *pPool, CDbInsta *pInstaDatabase);
	~CDbAccounts() = default;

	class CSelectInt
	{
	public:
		std::optional<int> m_Value = std::nullopt;
		bool m_IsDone = false;
		bool IsDone() const { return m_IsDone; }
		char m_aQuery[2048] = "";

		const char *ValueAsString()
		{
			if(!IsDone())
			{
				str_copy(m_aValueStrBuf, "(pending)");
				return m_aValueStrBuf;
			}
			if(!m_Value.has_value())
			{
				str_copy(m_aValueStrBuf, "(error)");
				return m_aValueStrBuf;
			}
			str_format(m_aValueStrBuf, sizeof(m_aValueStrBuf), "%d", m_Value.value());
			return m_aValueStrBuf;
		}

	private:
		char m_aValueStrBuf[512] = "";
	};
	std::vector<CSelectInt> m_vSelectInts;
	CSelectInt SelectInt(const char *pQuery);

	void CreateTable();

	// TODO: should register and login really be shared?
	//       should the argument be a union struct?
	//
	// ratelimited per player account requests
	void ChatCmd(int ClientId, const char *pUsername, const char *pDisplayName, const char *pOldPassword, const char *pNewPassword, CAccountChatCmd RequestType);

	// for now only used for resetting passwords
	// can in the future also be used to
	// - freeze
	// - unfreeze
	// - force logout if stuck
	// - dump account info
	void RconCmd(int ClientId, const char *pUsername, const char *pPassword, EAccountRconCmd RequestType);

	// unratelimited management request
	void SaveAndLogout(class CPlayer *pPlayer, const char *pSuccessMessage);

	// performs only one sql query to set all accounts to logged out in the db
	// does not actually operate on in game player instances
	// you still have to logout all CPlayer instances
	void LogoutAllOnCurrentServer();

	// check if the pName is claimed with the /claimname
	// command by a account
	// and find the account name that claimed it
	//
	// returns true on successful sql queue
	// returns false if it failed to lookup the name (it should block the name change and retry in that case)
	bool CheckNameClaimed(int ClientId, const char *pName);
};

#endif

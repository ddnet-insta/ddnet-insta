#include <base/log.h>
#include <base/system.h>
#include <cstdlib>
#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/account.h>
#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/password_hash.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/player.h>
#include <thread>

#include "sql_accounts.h"

class IDbConnection;

// TODO: scroll through this entire file and check all "true" "false" returns and also the if statements that read them
// TODO: properly separate the switch statements from the methods

CAccountPlayerResult::CAccountPlayerResult()
{
	SetVariant(EAccountPlayerRequestType::DIRECT);
	m_Account.Reset();
}

CAccountManagementResult::CAccountManagementResult(const char *pSuccessMessage)
{
	str_copy(m_aMessage, pSuccessMessage);
}

CAccountRconCmdResult::CAccountRconCmdResult(int UniqueClientId) :
	m_UniqueClientId(UniqueClientId)
{
	for(auto &aMessage : m_aaMessages)
		aMessage[0] = 0;
}

void CAccountPlayerResult::SetVariant(EAccountPlayerRequestType RequestType)
{
	m_MessageKind = RequestType;
	switch(RequestType)
	{
	case EAccountPlayerRequestType::CHAT_CMD_REGISTER:
	case EAccountPlayerRequestType::CHAT_CMD_LOGIN:
	case EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD:
	case EAccountPlayerRequestType::CHAT_CMD_SLOW_ACCOUNT_OPERATION:
	case EAccountPlayerRequestType::DIRECT:
	case EAccountPlayerRequestType::ALL:
	case EAccountPlayerRequestType::LOG_INFO:
	case EAccountPlayerRequestType::LOG_ERROR:
	case EAccountPlayerRequestType::LOGIN_FAILED:
		for(auto &aMessage : m_aaMessages)
			aMessage[0] = 0;
		break;
	case EAccountPlayerRequestType::BROADCAST:
		m_aBroadcast[0] = 0;
		break;
		break;
	}
}

bool CSqlAccounts::CreateAccountsTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w == Write::NORMAL_FAILED)
	{
		if(!MysqlAvailable())
		{
			log_error("sql-thread", "failed to create fastcap table! Make sure to compile with MySQL support if you want to use accounts");
			return false;
		}

		dbg_assert(false, "CreateAccountsTableThread failed to write");
		return false;
	}

	char aBuf[4096];
	str_format(aBuf, sizeof(aBuf),
		"CREATE TABLE IF NOT EXISTS accounts%s("
		" username          CHAR(%d)      NOT NULL,"
		" password          CHAR(%d)      NOT NULL,"
		" logged_in         INTEGER       DEFAULT 0,"
		" locked            INTEGER       DEFAULT 0,"
		" server_ip         CHAR(64)      NOT NULL DEFAULT '',"
		" server_port       INTEGER       NOT NULL DEFAULT 0,"
		" display_name      VARCHAR(%d)   COLLATE %s NOT NULL DEFAULT '',"
		" contact           CHAR(%d)      NOT NULL DEFAULT '',"
		" pin               INTEGER       NOT NULL DEFAULT 0,"
		" register_ip       CHAR(64)      NOT NULL,"
		" last_login        TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		" created_at        TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		" updated_at        TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		"PRIMARY KEY (username)"
		");",
		w == Write::NORMAL ? "" : "_backup",
		MAX_USERNAME_LENGTH,
		MAX_HASH_WITH_SALT_LENGTH,
		MAX_NAME_LENGTH_SQL,
		pSqlServer->BinaryCollate(),
		MAX_CONTACT_LENGTH);

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->Print();
	int NumInserted;
	return pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize);
}

bool CSqlAccounts::AccountWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
	{
		// could write to backup database here
		return true;
	}

	const auto *pData = dynamic_cast<const CSqlPlayerAccountRequest *>(pGameData);
	auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());
	char aBuf[4096];

	if(pData->m_RequestType == EAccountPlayerRequestType::CHAT_CMD_LOGIN)
	{
		if(!LoadAccount(pSqlServer, pData->m_aUsername, &pResult->m_Account, pError, ErrorSize))
		{
			pResult->m_MessageKind = EAccountPlayerRequestType::LOGIN_FAILED;
			str_copy(pResult->m_aaMessages[0], "Wrong username or password"); // wrong username
			return true;
		}

		if(pResult->m_Account.IsLoggedIn())
		{
			pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
			str_copy(pResult->m_aaMessages[0], "This account is already logged in");
			return true;
		}
		if(pResult->m_Account.IsLocked())
		{
			pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
			str_copy(pResult->m_aaMessages[0], "This account is locked");
			return true;
		}

		char aHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
		char aSalt[MAX_SALT_LENGTH];
		pass_get_salt(pResult->m_Account.m_aHashWithSalt, aSalt, sizeof(aSalt));
		pass_gen_hash_with_salt(aSalt, pData->m_aOldPassword, aHashWithSalt, sizeof(aHashWithSalt));

		if(str_comp(aHashWithSalt, pResult->m_Account.m_aHashWithSalt))
		{
			pResult->m_MessageKind = EAccountPlayerRequestType::LOGIN_FAILED;
			str_copy(pResult->m_aaMessages[0], "Wrong username or password"); // wrong password
			return true;
		}

		if(pData->m_DebugStats > 1)
		{
			log_debug("sql-thread", "cid=%d correct password for account '%s'", pData->m_ClientId, pResult->m_Account.m_aUsername);
		}

		// set logged in
		str_copy(
			aBuf,
			"UPDATE accounts "
			"SET"
			" logged_in = 1,"
			" server_ip = ?, server_port = ?,"
			" last_login = CURRENT_TIMESTAMP "
			"WHERE username = ?;");

		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			log_error("sql-thread", "prepare update failed query=%s", aBuf);
			return false;
		}

		int Offset = 1;
		pSqlServer->BindString(Offset++, pData->m_aServerIp);
		pSqlServer->BindInt(Offset++, pData->m_ServerPort);
		pSqlServer->BindString(Offset++, pData->m_aUsername);
		pSqlServer->Print();

		int NumUpdated;
		if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
		{
			return false;
		}

		if(NumUpdated != 1)
		{
			log_error("sql-thread", "affected %d rows when trying to update the account of one player!", NumUpdated);
			dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
			return false;
		}

		// success login
		pResult->m_MessageKind = pData->m_RequestType;
	}
	else if(pData->m_RequestType == EAccountPlayerRequestType::CHAT_CMD_REGISTER)
	{
		// check taken username
		str_copy(
			aBuf,
			"SELECT"
			" username "
			"FROM accounts "
			"WHERE username = ?;");
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			log_error("sql-thread", "prepare failed query: %s", aBuf);
			return false;
		}
		pSqlServer->BindString(1, pData->m_aUsername);
		pSqlServer->Print();

		bool End;
		if(!pSqlServer->Step(&End, pError, ErrorSize))
		{
			log_error("sql-thread", "step failed query: %s", aBuf);
			return false;
		}

		if(!End)
		{
			pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
			str_copy(pResult->m_aaMessages[0], "Username already taken");
			return true;
		}

		// check ip limit

		const char *pSixHoursAgo = "";
		if(CSqlStats::DetectBackend(pSqlServer) == ESqlBackend::MYSQL)
			pSixHoursAgo = "date_sub(now(), interval 6 hour)";
		else if(CSqlStats::DetectBackend(pSqlServer) == ESqlBackend::SQLITE3)
			pSixHoursAgo = "datetime('now', '-6 hours')";
		else
			log_error("sql-thread", "unsupported sql backend for ratelimits");

		str_format(
			aBuf,
			sizeof(aBuf),
			"SELECT"
			" count(*) "
			"FROM accounts "
			"WHERE register_ip = ? and created_at > %s;",
			pSixHoursAgo);

		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			log_error("sql-thread", "prepare failed query: %s", aBuf);
			return false;
		}
		pSqlServer->BindString(1, pData->m_aUserIpAddr);
		pSqlServer->Print();

		if(!pSqlServer->Step(&End, pError, ErrorSize))
		{
			log_error("sql-thread", "step failed query: %s", aBuf);
			return false;
		}

		if(!End)
		{
			int AccountsCreated = pSqlServer->GetInt(1);
			if(AccountsCreated)
			{
				log_info("sql-thread", "cid=%d register blocked (%s created %d accounts in the last 6 hours)", pData->m_ClientId, pData->m_aUserIpAddr, AccountsCreated);
				pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
				str_copy(pResult->m_aaMessages[0], "You already created an account. Try again later.");
				return true;
			}
		}

		// all good insert account
		if(pData->m_DebugStats > 1)
			log_debug("sql-thread", "cid=%d inserting new account '%s' ...", pData->m_ClientId, pData->m_aUsername);
		str_format(
			aBuf,
			sizeof(aBuf),
			"INSERT INTO accounts("
			" username, password, "
			" register_ip, "
			" created_at, updated_at "
			") VALUES ("
			" ?, ?,"
			" ?,"
			" %s, %s"
			");",
			pSqlServer->InsertTimestampAsUtc(),
			pSqlServer->InsertTimestampAsUtc());

		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			log_error("sql-thread", "prepare insert failed query=%s", aBuf);
			return false;
		}

		if(pData->m_DebugStats > 1)
			log_debug("sql-thread", "inserted query: %s", aBuf);

		char aHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
		char aSalt[MAX_SALT_LENGTH];
		pass_gen_salt(aSalt, sizeof(aSalt));
		pass_gen_hash_with_salt(aSalt, pData->m_aOldPassword, aHashWithSalt, sizeof(aHashWithSalt));

		int Offset = 1;
		pSqlServer->BindString(Offset++, pData->m_aUsername);
		pSqlServer->BindString(Offset++, aHashWithSalt);
		pSqlServer->BindString(Offset++, pData->m_aUserIpAddr);
		pSqlServer->BindString(Offset++, pData->m_aTimestamp);
		pSqlServer->BindString(Offset++, pData->m_aTimestamp);
		pSqlServer->Print();

		int NumInserted;
		if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		{
			return false;
		}

		// success register
		pResult->m_MessageKind = pData->m_RequestType;
	}
	else if(pData->m_RequestType == EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD)
		return ChangePasswordRequest(pSqlServer, pGameData, pError, ErrorSize);
	else if(pData->m_RequestType == EAccountPlayerRequestType::CHAT_CMD_SLOW_ACCOUNT_OPERATION)
	{
		log_info("sql-thread", "starting slooooooooooooooooooooooooooooooooow debug operation ...");
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10000ms);
		log_info("sql-thread", "finished slow debug operation");
		pResult->m_MessageKind = EAccountPlayerRequestType::CHAT_CMD_SLOW_ACCOUNT_OPERATION;
		str_copy(pResult->m_aaMessages[0], "slow debug operation reached main thread");
	}
	else
	{
		// TODO: this should be a switch instead
		log_error("sql-thread", "invalid request type");
	}
	return true;
}

bool CSqlAccounts::AccountSaveAndLogoutWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
	{
		// could save account to backup database here
		return true;
	}

	const auto *pData = dynamic_cast<const CSqlPlayerAccountData *>(pGameData);
	auto *pResult = dynamic_cast<CAccountManagementResult *>(pGameData->m_pResult.get());

	log_info("sql-thread", "logging out account '%s'", pData->m_Account.m_aUsername);
	if(!SetAccountInt(pSqlServer, pData->m_Account.m_aUsername, "logged_in", 0, pError, ErrorSize))
	{
		str_copy(pResult->m_aMessage, "Logout failed");
		return false;
	}
	return true;
}

bool CSqlAccounts::AccountRconCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
	{
		// writing to backup db makes no sense
		return true;
	}

	const auto *pData = dynamic_cast<const CSqlPlayerAccountRconCmdData *>(pGameData);
	auto *pResult = dynamic_cast<CAccountRconCmdResult *>(pGameData->m_pResult.get());
	pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_ERROR;
	str_copy(pResult->m_aaMessages[0], "Something went wrong");

	char aBuf[4096];

	switch(pData->m_RequestType)
	{
	// these are only used for output not for input
	case EAccountRconPlayerRequestType::LOG_ERROR:
	case EAccountRconPlayerRequestType::LOG_INFO:
	case EAccountRconPlayerRequestType::DIRECT:
	case EAccountRconPlayerRequestType::ALL:
		// noop: to please tooling that all switch branches are exhausted
		break;
	case EAccountRconPlayerRequestType::ACC_SET_PASSWORD:
		str_copy(
			aBuf,
			"SELECT"
			" password,"
			" logged_in, locked "
			"FROM accounts "
			"WHERE username = ?;");
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			log_error("sql-thread", "prepare failed query: %s", aBuf);
			return false;
		}
		pSqlServer->BindString(1, pData->m_aUsername);
		pSqlServer->Print();

		bool End;
		if(!pSqlServer->Step(&End, pError, ErrorSize))
		{
			log_error("sql-thread", "step failed query: %s", aBuf);
			return false;
		}

		if(End)
		{
			pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_ERROR;
			str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]), "account '%s' not found", pData->m_aUsername);
			return true;
		}

		if(!SetPassword(pSqlServer, pData->m_aUsername, pData->m_aPassword, pError, ErrorSize))
		{
			pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_ERROR;
			str_copy(pResult->m_aaMessages[0], "criticial database error");
			return false;
		}

		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"admin '%s' set password for account '%s'",
			pData->m_aAdminName,
			pData->m_aUsername);

		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
		break;
	case EAccountRconPlayerRequestType::ACC_LOGOUT:
		return ForceLogout(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconPlayerRequestType::ACC_LOCK:
		return LockAccount(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconPlayerRequestType::ACC_UNLOCK:
		return UnlockAccount(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconPlayerRequestType::ACC_INFO:
		return AccountInfo(pSqlServer, pData, pResult, pError, ErrorSize);
	}

	return false;
}

bool CSqlAccounts::ForceLogout(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconPlayerRequestType::ACC_LOGOUT, "invalid request type");

	CAccount Account;
	if(!LoadAccount(pSqlServer, pData->m_aUsername, &Account, pError, ErrorSize))
	{
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_ERROR;
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Force logout failed. There is no account with the username '%s'", pData->m_aUsername);
		return true;
	}

	if(!Account.IsLoggedIn())
	{
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_ERROR;
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Force logout failed. Account '%s' is already logged out.", pData->m_aUsername);
		return true;
	}

	// if the account is logged in on another server that is still running
	// we can not ensure the account gets logged out there
	// to avoid causing bugs the admin has to be on the same server
	if(str_comp(Account.ServerIp(), pData->m_aServerIp) || Account.ServerPort() != pData->m_ServerPort)
	{
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_ERROR;
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Force logout failed. Account '%s' is logged in on server %s:%d but you are on %s:%d",
			pData->m_aUsername,
			Account.ServerIp(),
			Account.ServerPort(),
			pData->m_aServerIp,
			pData->m_ServerPort);
		return true;
	}

	// force logout bugged account
	// this branch should only be hit if a account got into a bad state because of a bug
	// all regular cases should already have logged out and returned earlier

	if(!SetAccountInt(pSqlServer, pData->m_aUsername, "logged_in", 0, pError, ErrorSize))
	{
		return false;
	}

	str_format(
		pResult->m_aaMessages[0],
		sizeof(pResult->m_aaMessages[0]),
		"'%s' force logged out account '%s'",
		pData->m_aAdminName,
		pData->m_aUsername);
	pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
	return true;
}

bool CSqlAccounts::LockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconPlayerRequestType::ACC_LOCK, "invalid request type");

	CAccount Account;
	if(!LoadAccount(pSqlServer, pData->m_aUsername, &Account, pError, ErrorSize))
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"failed to lock account '%s' (username not found)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
		return true;
	}

	if(Account.m_IsLocked)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Account '%s' is already locked",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
		return true;
	}

	if(!SetAccountInt(pSqlServer, pData->m_aUsername, "locked", 1, pError, ErrorSize))
	{
		return false;
	}
	str_format(
		pResult->m_aaMessages[0],
		sizeof(pResult->m_aaMessages[0]),
		"admin '%s' locked account '%s'",
		pData->m_aAdminName,
		pData->m_aUsername);
	if(Account.m_IsLoggedIn)
	{
		str_format(
			pResult->m_aaMessages[1],
			sizeof(pResult->m_aaMessages[1]),
			"Warning account '%s' is still logged in on the server %s:%d",
			pData->m_aUsername,
			Account.m_aServerIp,
			Account.m_ServerPort);
		str_format(
			pResult->m_aaMessages[2],
			sizeof(pResult->m_aaMessages[2]),
			"You have to manually go to that server and call 'acc_logout %s'",
			pData->m_aUsername);
	}

	pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
	return true;
}

bool CSqlAccounts::UnlockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconPlayerRequestType::ACC_UNLOCK, "invalid request type");

	CAccount Account;
	if(!LoadAccount(pSqlServer, pData->m_aUsername, &Account, pError, ErrorSize))
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"failed to unlock account '%s' (username not found)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
		return true;
	}

	if(!Account.m_IsLocked)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Account '%s' is not locked",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
		return true;
	}

	if(!SetAccountInt(pSqlServer, pData->m_aUsername, "locked", 0, pError, ErrorSize))
	{
		return false;
	}
	str_format(
		pResult->m_aaMessages[0],
		sizeof(pResult->m_aaMessages[0]),
		"admin '%s' unlocked account '%s'",
		pData->m_aAdminName,
		pData->m_aUsername);

	pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
	return true;
}

bool CSqlAccounts::AccountInfo(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconPlayerRequestType::ACC_INFO, "invalid request type");

	CAccount Account;
	if(!LoadAccount(pSqlServer, pData->m_aUsername, &Account, pError, ErrorSize))
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"account with username '%s' not found",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconPlayerRequestType::LOG_INFO;
		return true;
	}

	str_copy(pResult->m_aUsername, pData->m_aUsername); // could also be initialized for all types not onyl acc info
	pResult->m_Account = Account;
	pResult->m_MessageKind = EAccountRconPlayerRequestType::ACC_INFO;
	return true;
}

bool CSqlAccounts::ChangePasswordRequest(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerAccountRequest *>(pGameData);
	auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());
	dbg_assert(pData->m_RequestType == EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD, "ChangePassword called with wrong request type");
	pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
	str_copy(pResult->m_aaMessages[0], "Something went wrong");

	char aBuf[4096];
	str_copy(
		aBuf,
		"SELECT"
		" password,"
		" logged_in, locked "
		"FROM accounts "
		"WHERE username = ?;");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aUsername);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		log_error("sql-thread", "step failed query: %s", aBuf);
		return false;
	}

	if(End)
	{
		pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
		str_copy(pResult->m_aaMessages[0], "Account not found please contact an administrator");
		log_error("sql-thread", "change password request did not find account '%s'", pData->m_aUsername);
		return false;
	}

	// read account data
	int Offset = 1;
	pSqlServer->GetString(Offset++, pResult->m_Account.m_aHashWithSalt, sizeof(pResult->m_Account.m_aHashWithSalt));
	int LoggedIn = pSqlServer->GetInt(Offset++);
	if(LoggedIn == 0)
	{
		pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
		str_copy(pResult->m_aaMessages[0], "Your account is not logged in. Try reconnecting.");
		return true;
	}
	int Locked = pSqlServer->GetInt(Offset++);
	if(Locked)
	{
		pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
		str_copy(pResult->m_aaMessages[0], "Your account is locked.");
		return true;
	}

	char aHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
	char aSalt[MAX_SALT_LENGTH];
	pass_get_salt(pResult->m_Account.m_aHashWithSalt, aSalt, sizeof(aSalt));
	pass_gen_hash_with_salt(aSalt, pData->m_aOldPassword, aHashWithSalt, sizeof(aHashWithSalt));

	if(str_comp(aHashWithSalt, pResult->m_Account.m_aHashWithSalt))
	{
		pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
		str_copy(pResult->m_aaMessages[0], "Wrong old password");
		return true;
	}

	// set password if old matched
	if(!SetPassword(pSqlServer, pData->m_aUsername, pData->m_aNewPassword, pError, ErrorSize))
	{
		pResult->m_MessageKind = EAccountPlayerRequestType::DIRECT;
		str_copy(pResult->m_aaMessages[0], "Criticial database error. Please contact an administrator.");
		return false;
	}
	pResult->m_MessageKind = EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD;
	return true;
}

bool CSqlAccounts::SetPassword(IDbConnection *pSqlServer, const char *pUsername, const char *pPassword, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_copy(
		aBuf,
		"UPDATE accounts "
		"SET"
		" password = ?,"
		" updated_at = CURRENT_TIMESTAMP "
		"WHERE username = ?;");

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare update failed query=%s", aBuf);
		return false;
	}

	char aHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
	char aSalt[MAX_SALT_LENGTH];
	pass_gen_salt(aSalt, sizeof(aSalt));
	pass_gen_hash_with_salt(aSalt, pPassword, aHashWithSalt, sizeof(aHashWithSalt));

	int Offset = 1;
	pSqlServer->BindString(Offset++, aHashWithSalt);
	pSqlServer->BindString(Offset++, pUsername);
	pSqlServer->Print();

	int NumUpdated;
	if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
	{
		return false;
	}

	if(NumUpdated != 1)
	{
		log_error("sql-thread", "affected %d rows when trying to update the account of one player!", NumUpdated);
		dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
		return false;
	}

	return true;
}

bool CSqlAccounts::LoadAccount(IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_copy(
		aBuf,
		"SELECT"
		" username, password,"
		" logged_in, locked,"
		" server_ip, server_port,"
		" display_name, contact, pin,"
		" register_ip "
		/* " last_login, created_at, updated_at " */ // TODO: load dates
		"FROM accounts "
		"WHERE username = ?;");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pUsername);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		log_error("sql-thread", "step failed query: %s", aBuf);
		return false;
	}

	if(End)
	{
		return false; // not a fatal error but no account loaded
	}

	if(pAccount)
	{
		int Offset = 1;
		pSqlServer->GetString(Offset++, pAccount->m_aUsername, sizeof(pAccount->m_aUsername));
		pSqlServer->GetString(Offset++, pAccount->m_aHashWithSalt, sizeof(pAccount->m_aHashWithSalt));
		pAccount->m_IsLoggedIn = pSqlServer->GetInt(Offset++) == 1;
		pAccount->m_IsLocked = pSqlServer->GetInt(Offset++) == 1;
		pSqlServer->GetString(Offset++, pAccount->m_aServerIp, sizeof(pAccount->m_aServerIp));
		pAccount->m_ServerPort = pSqlServer->GetInt(Offset++);
		pSqlServer->GetString(Offset++, pAccount->m_aDisplayName, sizeof(pAccount->m_aDisplayName));
		pSqlServer->GetString(Offset++, pAccount->m_aContact, sizeof(pAccount->m_aContact));
		pAccount->m_Pin = pSqlServer->GetInt(Offset++);
		pSqlServer->GetString(Offset++, pAccount->m_aRegiserIp, sizeof(pAccount->m_aRegiserIp));
	}

	return true;
}

bool CSqlAccounts::SetAccountInt(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, int Value, char *pError, int ErrorSize)
{
	char aBuf[1024];
	str_format(
		aBuf,
		sizeof(aBuf),
		"UPDATE accounts "
		"SET"
		" %s = ? "
		"WHERE username = ?;",
		pColumn);

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare update failed query=%s", aBuf);
		return false;
	}

	pSqlServer->BindInt(1, Value);
	pSqlServer->BindString(2, pUsername);
	pSqlServer->Print();

	int NumUpdated;
	if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
	{
		log_error("sql-thread", "update failed query=%s", aBuf);
		return false;
	}

	if(NumUpdated != 1)
	{
		log_error("sql-thread", "affected %d rows when trying to update the account of one player!", NumUpdated);
		dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
		return false;
	}

	return true;
}

bool CSqlAccounts::SetAccountString(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, const char *pValue, char *pError, int ErrorSize)
{
	char aBuf[1024];
	str_format(
		aBuf,
		sizeof(aBuf),
		"UPDATE accounts "
		"SET"
		" %s = ? "
		"WHERE username = ?;",
		pColumn);

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare update failed query=%s", aBuf);
		return false;
	}

	pSqlServer->BindString(1, pUsername);
	pSqlServer->BindString(2, pValue);
	pSqlServer->Print();

	int NumUpdated;
	if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
	{
		log_error("sql-thread", "update failed query=%s", aBuf);
		return false;
	}

	if(NumUpdated != 1)
	{
		log_error("sql-thread", "affected %d rows when trying to update the account of one player!", NumUpdated);
		dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
		return false;
	}

	return true;
}

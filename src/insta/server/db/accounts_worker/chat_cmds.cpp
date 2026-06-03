#include "accounts_worker.h"

#include <base/log.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/shared/protocol.h>

#include <insta/server/db/accounts_worker/chat_cmds.h>
#include <insta/server/ddnet_db_utils/ddnet_db_utils.h>
#include <insta/server/display_name.h>
#include <insta/server/password_hash.h>
#include <insta/server/strhelpers.h>

#include <thread>

bool CChatCmdsWorker::ChatCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
	{
		// could write to backup database here
		return true;
	}

	const auto *pData = dynamic_cast<const CSqlPlayerAccountRequest *>(pGameData);
	auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());

	switch(pData->m_RequestType)
	{
	// TODO: still a bit weird to switch over the result enum
	//       values here to determine the input type
	case EAccountChatCmd::DIRECT:
	case EAccountChatCmd::ALL:
	case EAccountChatCmd::BROADCAST:
	case EAccountChatCmd::LOG_INFO:
	case EAccountChatCmd::LOG_ERROR:
	case EAccountChatCmd::LOGIN_FAILED:
		break;
	case EAccountChatCmd::CHAT_CMD_LOGIN:
		return CmdLogin(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountChatCmd::CHAT_CMD_REGISTER:
		return CmdRegister(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountChatCmd::CHAT_CMD_CHANGE_PASSWORD:
		return CmdChangePassword(pSqlServer, pGameData, pError, ErrorSize);
	case EAccountChatCmd::CHAT_CMD_DISPLAY_NAME:
		return CmdDisplayName(pSqlServer, pGameData, pError, ErrorSize);
	case EAccountChatCmd::CHAT_CMD_LOCK_NAME:
		return CmdLockName(pSqlServer, pGameData, pError, ErrorSize);
	case EAccountChatCmd::CHAT_CMD_SLOW_ACCOUNT_OPERATION:
		return CmdSlowOperation(pSqlServer, pData, pResult, pError, ErrorSize);
	}

	log_error("sql-thread", "invalid request type %d", (int)pData->m_RequestType);
	return false;
}

CAccountPlayerResult::CAccountPlayerResult()
{
	SetVariant(EAccountChatCmd::DIRECT);
	m_Data.m_Account.Reset();
}

void CAccountPlayerResult::SetVariant(EAccountChatCmd RequestType)
{
	m_MessageKind = RequestType;
	switch(RequestType)
	{
	case EAccountChatCmd::CHAT_CMD_REGISTER:
	case EAccountChatCmd::CHAT_CMD_LOGIN:
	case EAccountChatCmd::CHAT_CMD_CHANGE_PASSWORD:
	case EAccountChatCmd::CHAT_CMD_DISPLAY_NAME:
	case EAccountChatCmd::CHAT_CMD_LOCK_NAME:
		m_Data.m_NameClaim.m_aDisplayName[0] = '\0';
		m_Data.m_NameClaim.m_aNameOwner[0] = '\0';
		m_Data.m_NameClaim.m_IsProtected = false;
		break;
	case EAccountChatCmd::CHAT_CMD_SLOW_ACCOUNT_OPERATION:
	case EAccountChatCmd::DIRECT:
	case EAccountChatCmd::ALL:
	case EAccountChatCmd::LOG_INFO:
	case EAccountChatCmd::LOG_ERROR:
	case EAccountChatCmd::LOGIN_FAILED:
		m_Data.m_LoginFailed.m_aError[0] = '\0';
		m_Data.m_LoginFailed.m_aUsername[0] = '\0';
		break;
	case EAccountChatCmd::BROADCAST:
		m_Data.m_aBroadcast[0] = 0;
		break;
		break;
	}
}

void CAccountPlayerResult::SetLoginFailed(const char *pUsername, const char *pError)
{
	m_MessageKind = EAccountChatCmd::LOGIN_FAILED;
	str_copy(m_Data.m_LoginFailed.m_aUsername, pUsername);
	str_copy(m_Data.m_LoginFailed.m_aError, pError);
}

bool CChatCmdsWorker::CmdLogin(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize)
{
	const char *pUsername = pData->m_aUsername;
	char aBuf[4096];
	EResult LoadResult = LoadAccount(pSqlServer, pUsername, &pResult->m_Data.m_Account, pData->m_vTables, pError, ErrorSize);
	switch(LoadResult)
	{
	case EResult::SUCCESS:
		break;
	case EResult::INVALID:
		pResult->SetLoginFailed(pUsername, "Wrong username or password"); // wrong username
		return true;
	case EResult::FATAL_ERROR:
		pResult->SetLoginFailed(pUsername, "Login failed because there was an internal database error");
		return false;
	}

	if(pResult->m_Data.m_Account.IsLoggedIn())
	{
		pResult->SetLoginFailed(pUsername, "This account is already logged in");
		return true;
	}
	if(pResult->m_Data.m_Account.IsLocked())
	{
		pResult->SetLoginFailed(pUsername, "This account is locked");
		return true;
	}

	char aHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
	char aSalt[MAX_SALT_LENGTH];
	pass_get_salt(pResult->m_Data.m_Account.m_aHashWithSalt, aSalt, sizeof(aSalt));
	pass_gen_hash_with_salt(aSalt, pData->m_aOldPassword, aHashWithSalt, sizeof(aHashWithSalt));

	if(str_comp(aHashWithSalt, pResult->m_Data.m_Account.m_aHashWithSalt))
	{
		pResult->SetLoginFailed(pUsername, "Wrong username or password"); // wrong password
		return true;
	}

	if(pData->m_DebugAccounts > 1)
	{
		log_debug("sql-thread", "cid=%d correct password for account '%s'", pData->m_ClientId, pResult->m_Data.m_Account.m_aUsername);
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
	return true;
}

bool CChatCmdsWorker::CmdRegister(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize)
{
	pResult->m_MessageKind = EAccountChatCmd::DIRECT;
	str_copy(pResult->m_Data.m_aaMessages[0], "Something went wrong");

	char aBuf[4096];
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
		str_copy(pResult->m_Data.m_aaMessages[0], "Register failed because there was an internal database error");
		return false;
	}
	pSqlServer->BindString(1, pData->m_aUsername);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		log_error("sql-thread", "step failed query: %s", aBuf);
		str_copy(pResult->m_Data.m_aaMessages[0], "Register failed because there was an internal database error");
		return false;
	}

	if(!End)
	{
		pResult->m_MessageKind = EAccountChatCmd::DIRECT;
		str_copy(pResult->m_Data.m_aaMessages[0], "Username already taken");
		return true;
	}

	// check ip limit

	const char *pSixHoursAgo = "";
	if(ddnet_db_utils::DetectBackend(pSqlServer) == ddnet_db_utils::ESqlBackend::MYSQL)
		pSixHoursAgo = "date_sub(now(), interval 6 hour)";
	else if(ddnet_db_utils::DetectBackend(pSqlServer) == ddnet_db_utils::ESqlBackend::SQLITE3)
		pSixHoursAgo = "datetime('now', '-6 hours')";
	else
		log_error("sql-thread", "unsupported sql backend for ratelimits");

	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT"
		" count(*) "
		"FROM accounts "
		"WHERE register_ip = ? and register_date > %s;",
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
			pResult->m_MessageKind = EAccountChatCmd::DIRECT;
			str_copy(pResult->m_Data.m_aaMessages[0], "You already created an account. Try again later.");
			return true;
		}
	}

	// all good insert account
	if(pData->m_DebugAccounts > 1)
		log_debug("sql-thread", "cid=%d inserting new account '%s' ...", pData->m_ClientId, pData->m_aUsername);
	str_format(
		aBuf,
		sizeof(aBuf),
		"INSERT INTO accounts("
		" username, password, "
		" register_ip, "
		" register_date"
		") VALUES ("
		" ?, ?,"
		" ?,"
		" %s"
		");",
		pSqlServer->InsertTimestampAsUtc());

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare insert failed query=%s", aBuf);
		return false;
	}

	if(pData->m_DebugAccounts > 1)
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
	pSqlServer->Print();

	int NumInserted;
	if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
	{
		return false;
	}

	// success register
	pResult->m_MessageKind = pData->m_RequestType;
	return true;
}

bool CChatCmdsWorker::CmdChangePassword(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerAccountRequest *>(pGameData);
	auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());
	dbg_assert(pData->m_RequestType == EAccountChatCmd::CHAT_CMD_CHANGE_PASSWORD, "ChangePassword called with wrong request type");
	pResult->m_MessageKind = EAccountChatCmd::DIRECT;
	str_copy(pResult->m_Data.m_aaMessages[0], "Something went wrong");

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
		pResult->m_MessageKind = EAccountChatCmd::DIRECT;
		str_copy(pResult->m_Data.m_aaMessages[0], "Account not found please contact an administrator");
		log_error("sql-thread", "change password request did not find account '%s'", pData->m_aUsername);
		return false;
	}

	// read account data
	int Offset = 1;
	pSqlServer->GetString(Offset++, pResult->m_Data.m_Account.m_aHashWithSalt, sizeof(pResult->m_Data.m_Account.m_aHashWithSalt));
	int LoggedIn = pSqlServer->GetInt(Offset++);
	if(LoggedIn == 0)
	{
		pResult->m_MessageKind = EAccountChatCmd::DIRECT;
		str_copy(pResult->m_Data.m_aaMessages[0], "Your account is not logged in. Try reconnecting.");
		return true;
	}
	int Locked = pSqlServer->GetInt(Offset++);
	if(Locked)
	{
		pResult->m_MessageKind = EAccountChatCmd::DIRECT;
		str_copy(pResult->m_Data.m_aaMessages[0], "Your account is locked.");
		return true;
	}

	char aHashWithSalt[MAX_HASH_WITH_SALT_LENGTH];
	char aSalt[MAX_SALT_LENGTH];
	pass_get_salt(pResult->m_Data.m_Account.m_aHashWithSalt, aSalt, sizeof(aSalt));
	pass_gen_hash_with_salt(aSalt, pData->m_aOldPassword, aHashWithSalt, sizeof(aHashWithSalt));

	if(str_comp(aHashWithSalt, pResult->m_Data.m_Account.m_aHashWithSalt))
	{
		pResult->m_MessageKind = EAccountChatCmd::DIRECT;
		str_copy(pResult->m_Data.m_aaMessages[0], "Wrong old password");
		return true;
	}

	// set password if old matched
	if(!SetPassword(pSqlServer, pData->m_aUsername, pData->m_aNewPassword, pError, ErrorSize))
	{
		pResult->m_MessageKind = EAccountChatCmd::DIRECT;
		str_copy(pResult->m_Data.m_aaMessages[0], "Critical database error. Please contact an administrator.");
		return false;
	}
	pResult->m_MessageKind = EAccountChatCmd::CHAT_CMD_CHANGE_PASSWORD;
	return true;
}

bool CChatCmdsWorker::CmdDisplayName(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerAccountRequest *>(pGameData);
	auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());
	dbg_assert(pData->m_RequestType == EAccountChatCmd::CHAT_CMD_DISPLAY_NAME, "ChatCmdDisplayName called with wrong request type");
	pResult->m_MessageKind = EAccountChatCmd::DIRECT;
	str_copy(pResult->m_Data.m_aaMessages[0], "Something went wrong");

	CDisplayNameOwner Owner;
	if(!GetDisplayNameOwner(pSqlServer, pData->m_aDisplayName, &Owner, pError, ErrorSize))
	{
		str_format(pResult->m_Data.m_aaMessages[0], sizeof(pResult->m_Data.m_aaMessages[0]), "Failed to claim name '%s' because of an database error.", pData->m_aDisplayName);
		return true;
	}
	if(Owner.m_aUsername[0] != '\0' && str_comp(Owner.m_aUsername, pData->m_aUsername))
	{
		str_format(pResult->m_Data.m_aaMessages[0], sizeof(pResult->m_Data.m_aaMessages[0]), "The display name '%s' is already claimed by another account.", pData->m_aDisplayName);
		return true;
	}
	if(Owner.m_aUsername[0] != '\0' && str_comp(Owner.m_aUsername, pData->m_aUsername) == 0)
	{
		str_format(pResult->m_Data.m_aaMessages[0], sizeof(pResult->m_Data.m_aaMessages[0]), "You already claimed the display name '%s'.", pData->m_aDisplayName);
		return true;
	}

	char aSkel[MAX_NAME_LENGTH * 2];
	str_utf8_to_skeleton_str(pData->m_aDisplayName, aSkel, sizeof(aSkel));
	log_info("sql-thread", "claiming name '%s' with skeleton '%s'", pData->m_aDisplayName, aSkel);

	SetAccountString(pSqlServer, pData->m_aUsername, "display_name", pData->m_aDisplayName, pError, ErrorSize);
	SetAccountString(pSqlServer, pData->m_aUsername, "display_name_skel", aSkel, pError, ErrorSize);
	if(Owner.m_IsProtected == false)
	{
		if(!SetAccountInt(pSqlServer, pData->m_aUsername, "name_protected", 1, pError, ErrorSize))
			return false;
	}

	str_copy(pResult->m_Data.m_NameClaim.m_aDisplayName, pData->m_aDisplayName);
	str_copy(pResult->m_Data.m_NameClaim.m_aNameOwner, pData->m_aUsername);
	pResult->m_Data.m_NameClaim.m_IsProtected = true;

	pResult->m_MessageKind = EAccountChatCmd::CHAT_CMD_DISPLAY_NAME;
	return true;
}

bool CChatCmdsWorker::CmdLockName(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerAccountRequest *>(pGameData);
	auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());
	dbg_assert(pData->m_RequestType == EAccountChatCmd::CHAT_CMD_LOCK_NAME, "ChatCmdLockName called with wrong request type");
	pResult->m_MessageKind = EAccountChatCmd::DIRECT;
	str_copy(pResult->m_Data.m_aaMessages[0], "Something went wrong");

	CDisplayNameOwner Owner;
	if(!GetDisplayNameOwner(pSqlServer, pData->m_aDisplayName, &Owner, pError, ErrorSize))
	{
		str_format(pResult->m_Data.m_aaMessages[0], sizeof(pResult->m_Data.m_aaMessages[0]), "Failed to lock name '%s' because of an database error.", pData->m_aDisplayName);
		return true;
	}
	if(Owner.m_aUsername[0] != '\0' && str_comp(Owner.m_aUsername, pData->m_aUsername))
	{
		str_format(pResult->m_Data.m_aaMessages[0], sizeof(pResult->m_Data.m_aaMessages[0]), "Tried to lock name '%s' which is owned by another account.", pData->m_aDisplayName);
		return true;
	}

	// toggle
	int Protected = Owner.m_IsProtected ? 0 : 1;

	if(!SetAccountInt(pSqlServer, pData->m_aUsername, "name_protected", Protected, pError, ErrorSize))
		return false;

	str_copy(pResult->m_Data.m_NameClaim.m_aDisplayName, pData->m_aDisplayName);
	str_copy(pResult->m_Data.m_NameClaim.m_aNameOwner, pData->m_aUsername);
	pResult->m_Data.m_NameClaim.m_IsProtected = Protected;

	pResult->m_MessageKind = EAccountChatCmd::CHAT_CMD_LOCK_NAME;
	return true;
}

bool CChatCmdsWorker::CmdSlowOperation(IDbConnection *pSqlServer, const CSqlPlayerAccountRequest *pData, CAccountPlayerResult *pResult, char *pError, int ErrorSize)
{
	log_info("sql-thread", "starting slooooooooooooooooooooooooooooooooow debug operation ...");
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(10000ms);
	log_info("sql-thread", "finished slow debug operation");
	pResult->m_MessageKind = EAccountChatCmd::CHAT_CMD_SLOW_ACCOUNT_OPERATION;
	str_copy(pResult->m_Data.m_aaMessages[0], "slow debug operation reached main thread");
	return true;
}

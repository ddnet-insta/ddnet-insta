#include "accounts_worker.h"

#include <engine/server/databases/connection.h>
#include <engine/shared/config.h>

#include <insta/server/ddnet_db_utils/ddnet_db_utils.h>
#include <insta/server/display_name.h>
#include <insta/server/password_hash.h>
#include <insta/server/strhelpers.h>

#include <cstdint>

CAccountPlayerResult::CAccountPlayerResult()
{
	SetVariant(EAccountChatCmd::DIRECT);
	m_Data.m_Account.Reset();
}

CAccountManagementResult::CAccountManagementResult(const char *pSuccessMessage)
{
	str_copy(m_aMessage, pSuccessMessage);
}

CAccountRconCmdResult::CAccountRconCmdResult(uint32_t UniqueClientId) :
	m_UniqueClientId(UniqueClientId)
{
	for(auto &aMessage : m_aaMessages)
		aMessage[0] = 0;
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

bool CAccountsWorker::CheckNameClaimedWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlCheckNameClaimRequest *>(pGameData);
	auto *pResult = dynamic_cast<CCheckNameClaimResult *>(pGameData->m_pResult.get());

	if(!CAccountsWorker::GetDisplayNameOwner(pSqlServer, pData->m_aDisplayName, &pResult->m_Owner, pError, ErrorSize))
	{
		return false;
	}
	return true;
}

bool CAccountsWorker::AccountSaveAndLogoutWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
	{
		// could save account to backup database here
		return true;
	}

	const auto *pData = dynamic_cast<const CSqlPlayerAccountData *>(pGameData);
	auto *pResult = dynamic_cast<CAccountManagementResult *>(pGameData->m_pResult.get());

	if(g_Config.m_SvDebugAccounts)
		log_info("sql-thread", "logging out account '%s'", pData->m_Account.m_aUsername);
	if(!SetAccountInt(pSqlServer, pData->m_Account.m_aUsername, "logged_in", 0, pError, ErrorSize))
	{
		str_copy(pResult->m_aMessage, "Logout failed (error code 1)");
		return false;
	}

	if(!CExtraAccountTableController::Save(pSqlServer, pData->m_Account.Id(), &pData->m_Account, pData->m_vTables, pError, ErrorSize))
	{
		str_copy(pResult->m_aMessage, "Logout failed (error code 2)");
		return false;
	}

	return true;
}

bool CAccountsWorker::LogoutAllAccountsOnCurrentServerThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
		return true;

	const CSqlLogoutAllRequest *pData = dynamic_cast<const CSqlLogoutAllRequest *>(pGameData);
	if(g_Config.m_SvDebugAccounts > 1)
		log_info("sql-thread", "logging out all accounts on server '%s:%d'", pData->m_aServerIp, pData->m_ServerPort);

	char aBuf[4096];
	str_copy(
		aBuf,
		"UPDATE accounts "
		"SET logged_in = 0 "
		"WHERE server_ip = ? AND server_port = ? AND logged_in = 1;");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aServerIp);
	pSqlServer->BindInt(2, pData->m_ServerPort);
	pSqlServer->Print();

	int NumUpdated;
	if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
	{
		return false;
	}

	if(NumUpdated || g_Config.m_SvDebugAccounts > 1)
		log_info("sql-thread", "logged out %d old accounts (logout all cleanup)", NumUpdated);
	return true;
}

bool CAccountsWorker::ForceLogout(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconCmd::ACC_LOGOUT, "invalid request type");

	CAccount Account;
	if(LoadAccount(pSqlServer, pData->m_aUsername, &Account, {}, pError, ErrorSize) != EResult::SUCCESS)
	{
		pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Force logout failed. There is no account with the username '%s'", pData->m_aUsername);
		return true;
	}

	if(!Account.IsLoggedIn())
	{
		pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
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
		pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
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
	pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
	return true;
}

bool CAccountsWorker::LockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconCmd::ACC_LOCK, "invalid request type");

	CAccount Account;
	EResult LoadResult = LoadAccount(pSqlServer, pData->m_aUsername, &Account, {}, pError, ErrorSize);
	if(LoadResult == EResult::INVALID)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"failed to lock account '%s' (username not found)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
		return true;
	}
	else if(LoadResult == EResult::FATAL_ERROR)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"failed to lock account '%s' (database error)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
		return true;
	}

	if(Account.m_IsLocked)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Account '%s' is already locked",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
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

	pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
	return true;
}

bool CAccountsWorker::UnlockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconCmd::ACC_UNLOCK, "invalid request type");

	CAccount Account;
	EResult LoadResult = LoadAccount(pSqlServer, pData->m_aUsername, &Account, {}, pError, ErrorSize);
	if(LoadResult == EResult::INVALID)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"failed to unlock account '%s' (username not found)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
		return true;
	}
	else if(LoadResult == EResult::FATAL_ERROR)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"failed to unlock account '%s' (database error)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
		return true;
	}

	if(!Account.m_IsLocked)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"Account '%s' is not locked",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
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

	pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
	return true;
}

bool CAccountsWorker::AccountInfo(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconCmd::ACC_INFO, "invalid request type");

	CAccount Account;
	EResult LoadResult = LoadAccount(pSqlServer, pData->m_aUsername, &Account, {}, pError, ErrorSize);
	if(LoadResult == EResult::INVALID)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"account with username '%s' not found",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
		return true;
	}
	else if(LoadResult == EResult::FATAL_ERROR)
	{
		str_format(
			pResult->m_aaMessages[0],
			sizeof(pResult->m_aaMessages[0]),
			"account with username '%s' not found (database error)",
			pData->m_aUsername);
		pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
		return true;
	}

	str_copy(pResult->m_aUsername, pData->m_aUsername); // could also be initialized for all types not only acc info
	pResult->m_Account = Account;
	pResult->m_MessageKind = EAccountRconCmd::ACC_INFO;
	return true;
}

bool CAccountsWorker::SetPassword(IDbConnection *pSqlServer, const char *pUsername, const char *pPassword, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_copy(
		aBuf,
		"UPDATE accounts "
		"SET"
		" password = ? "
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

CAccountsWorker::EResult CAccountsWorker::LoadAccount(IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)
{
	char aLastLogin[512];
	char aRegisterDate[512];
	pSqlServer->ToUnixTimestamp("last_login", aLastLogin, sizeof(aLastLogin));
	pSqlServer->ToUnixTimestamp("register_date", aRegisterDate, sizeof(aRegisterDate));

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT"
		" id, username, password,"
		" logged_in, locked,"
		" server_ip, server_port,"
		" display_name, display_name_skel, name_protected,"
		" contact, pin,"
		" register_ip, "
		" %s, %s " // last_login, register_date
		"FROM accounts "
		"WHERE username = ?;",
		aLastLogin,
		aRegisterDate);
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", aBuf);
		return EResult::FATAL_ERROR;
	}
	pSqlServer->BindString(1, pUsername);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		log_error("sql-thread", "step failed query: %s", aBuf);
		return EResult::FATAL_ERROR;
	}

	if(End)
	{
		return EResult::INVALID;
	}

	if(pAccount)
	{
		int Offset = 1;
		pAccount->m_Id = pSqlServer->GetInt(Offset++);
		pSqlServer->GetString(Offset++, pAccount->m_aUsername, sizeof(pAccount->m_aUsername));
		pSqlServer->GetString(Offset++, pAccount->m_aHashWithSalt, sizeof(pAccount->m_aHashWithSalt));
		pAccount->m_IsLoggedIn = pSqlServer->GetInt(Offset++) == 1;
		pAccount->m_IsLocked = pSqlServer->GetInt(Offset++) == 1;
		pSqlServer->GetString(Offset++, pAccount->m_aServerIp, sizeof(pAccount->m_aServerIp));
		pAccount->m_ServerPort = pSqlServer->GetInt(Offset++);
		pSqlServer->GetString(Offset++, pAccount->m_aDisplayName, sizeof(pAccount->m_aDisplayName));
		pSqlServer->GetString(Offset++, pAccount->m_aDisplayNameSkel, sizeof(pAccount->m_aDisplayNameSkel));
		pAccount->m_IsNameProtected = pSqlServer->GetInt(Offset++) == 1;
		pSqlServer->GetString(Offset++, pAccount->m_aContact, sizeof(pAccount->m_aContact));
		pAccount->m_Pin = pSqlServer->GetOptionalInt(Offset++);
		pSqlServer->GetString(Offset++, pAccount->m_aRegisterIp, sizeof(pAccount->m_aRegisterIp));
		pAccount->m_LastLogin = pSqlServer->GetOptionalInt64(Offset++);
		pAccount->m_RegisterDate = pSqlServer->GetInt64(Offset++);

		if(!CExtraAccountTableController::Load(pSqlServer, pAccount->Id(), pAccount, vTables, pError, ErrorSize))
		{
			log_error("sql-thread", "failed to load extra account data, for details check the errors above");
			return EResult::FATAL_ERROR;
		}
	}

	return EResult::SUCCESS;
}

bool CAccountsWorker::GetDisplayNameOwner(IDbConnection *pSqlServer, const char *pDisplayName, CDisplayNameOwner *pOwner, char *pError, int ErrorSize)
{
	pOwner->Reset();
	str_copy(pOwner->m_aDisplayName, pDisplayName);

	char aDisplayNameSkeleton[MAX_NAME_LENGTH * 2] = "";
	if(!str_utf8_to_skeleton_str(pDisplayName, aDisplayNameSkeleton, sizeof(aDisplayNameSkeleton)))
	{
		str_format(pError, ErrorSize, "utf-8 confusable skeleton failed: %s", aDisplayNameSkeleton);
		return false;
	}

	char aBuf[1024];
	str_copy(
		aBuf,
		"SELECT"
		" username, name_protected "
		"FROM accounts "
		"WHERE display_name_skel = ?;");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, aDisplayNameSkeleton);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		log_error("sql-thread", "step failed query: %s", aBuf);
		return false;
	}

	if(!End)
	{
		pSqlServer->GetString(1, pOwner->m_aUsername, sizeof(pOwner->m_aUsername));
		pOwner->m_IsProtected = pSqlServer->GetInt(2) == 1;
	}
	return true;
}

bool CAccountsWorker::SetAccountInt(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, int Value, char *pError, int ErrorSize)
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

	// 0 updated rows can happen when the data did not change
	// this is probably a logic flaw but usually not a bad one
	// this used to assert but it was too annoying
	if(NumUpdated != 1 && NumUpdated != 0)
	{
		log_error("sql-thread", "affected %d rows when trying to update the account of one player!", NumUpdated);
		dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
		return false;
	}

	return true;
}

bool CAccountsWorker::SetAccountString(IDbConnection *pSqlServer, const char *pUsername, const char *pColumn, const char *pValue, char *pError, int ErrorSize)
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
	pSqlServer->BindString(1, pValue);
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
		log_error("sql-thread", "value='%s' username='%s'; %s", pValue, pUsername, aBuf);
		dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
		return false;
	}

	return true;
}

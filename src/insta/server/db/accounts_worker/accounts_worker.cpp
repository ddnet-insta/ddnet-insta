#include "accounts_worker.h"

#include <engine/server/databases/connection.h>
#include <engine/shared/config.h>

#include <insta/server/ddnet_db_utils/ddnet_db_utils.h>
#include <insta/server/display_name.h>
#include <insta/server/password_hash.h>
#include <insta/server/strhelpers.h>

#include <cstdint>

CAccountManagementResult::CAccountManagementResult(const char *pSuccessMessage)
{
	str_copy(m_aMessage, pSuccessMessage);
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
		"WHERE display_name_skel = ? or display_name = ? LIMIT 1;");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	// this is the primary search
	pSqlServer->BindString(1, aDisplayNameSkeleton);
	// fallback to direct match if something with the skeleton went wrong
	// this is useful for the staging server that had displaynames without confusable
	// support but will also be useful in the future when the skeleton version changes
	pSqlServer->BindString(2, pDisplayName);
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

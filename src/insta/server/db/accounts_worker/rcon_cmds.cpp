#include "rcon_cmds.h"

bool CRconCmdsWorker::RconCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
	{
		// writing to backup db makes no sense
		return true;
	}

	const auto *pData = dynamic_cast<const CSqlPlayerAccountRconCmdData *>(pGameData);
	auto *pResult = dynamic_cast<CAccountRconCmdResult *>(pGameData->m_pResult.get());
	pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
	str_copy(pResult->m_aaMessages[0], "Something went wrong");

	switch(pData->m_RequestType)
	{
	// these are only used for output not for input
	case EAccountRconCmd::LOG_ERROR:
	case EAccountRconCmd::LOG_INFO:
	case EAccountRconCmd::DIRECT:
	case EAccountRconCmd::ALL:
		// noop: to please tooling that all switch branches are exhausted
		break;
	case EAccountRconCmd::ACC_SET_PASSWORD:
		return CmdSetPassword(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_LOGOUT:
		return CmdForceLogout(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_LOCK:
		return CmdLockAccount(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_UNLOCK:
		return CmdUnlockAccount(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_INFO:
		return CmdAccountInfo(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_DISPLAYNAME:
		return CmdAccountDisplayname(pSqlServer, pData, pResult, pError, ErrorSize);
	}

	return false;
}

CAccountRconCmdResult::CAccountRconCmdResult(uint32_t UniqueClientId) :
	m_UniqueClientId(UniqueClientId)
{
	for(auto &aMessage : m_aaMessages)
		aMessage[0] = 0;
}

bool CRconCmdsWorker::CmdSetPassword(IDbConnection *pSqlServer, const struct CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconCmd::ACC_SET_PASSWORD, "invalid request type");

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
		pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
		str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]), "account '%s' not found", pData->m_aUsername);
		return true;
	}

	if(!SetPassword(pSqlServer, pData->m_aUsername, pData->m_aPassword, pError, ErrorSize))
	{
		pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
		str_copy(pResult->m_aaMessages[0], "critical database error");
		return false;
	}

	str_format(
		pResult->m_aaMessages[0],
		sizeof(pResult->m_aaMessages[0]),
		"admin '%s' set password for account '%s'",
		pData->m_aAdminName,
		pData->m_aUsername);

	pResult->m_MessageKind = EAccountRconCmd::LOG_INFO;
	return true;
}

bool CRconCmdsWorker::CmdForceLogout(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
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

bool CRconCmdsWorker::CmdLockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
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

bool CRconCmdsWorker::CmdUnlockAccount(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
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

bool CRconCmdsWorker::CmdAccountInfo(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
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

bool CRconCmdsWorker::CmdAccountDisplayname(IDbConnection *pSqlServer, const CSqlPlayerAccountRconCmdData *pData, CAccountRconCmdResult *pResult, char *pError, int ErrorSize)
{
	dbg_assert(pData->m_RequestType == EAccountRconCmd::ACC_DISPLAYNAME, "invalid request type");

	pResult->m_MessageKind = EAccountRconCmd::LOG_ERROR;
	str_copy(pResult->m_aaMessages[0], "Something went wrong");

	// yea pData->m_aUsername is the displayname its a bit nasty naming
	// because of the shared struct to avoid having too many similar unused fields
	const char *pDisplayname = pData->m_aUsername;

	CDisplayNameOwner Owner;
	if(!GetDisplayNameOwner(pSqlServer, pData->m_aUsername, &Owner, pError, ErrorSize))
	{
		str_copy(pResult->m_aaMessages[0], "Failed to lookup name because of an database error");
		return true;
	}

	// yes this is really ugly!
	// we pass the argument string for "delete" in a variable called password
	pResult->m_GotDeleted = str_comp(pData->m_aPassword, "delete") == 0;
	if(pResult->m_GotDeleted)
	{
		if(Owner.m_aUsername[0])
		{
			if(!SetAccountString(pSqlServer, Owner.m_aUsername, "display_name", "", pError, ErrorSize))
				return false;
			if(!SetAccountString(pSqlServer, Owner.m_aUsername, "display_name_skel", "", pError, ErrorSize))
				return false;
		}
		else
		{
			str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]), "Failed to remove ownership of unclaimed displayname '%s'", pDisplayname);
			return true;
		}
	}

	// will be an empty string if unclaimed
	str_copy(pResult->m_aUsername, Owner.m_aUsername);
	str_copy(pResult->m_aDisplayname, pDisplayname);
	pResult->m_MessageKind = EAccountRconCmd::ACC_DISPLAYNAME;

	return true;
}

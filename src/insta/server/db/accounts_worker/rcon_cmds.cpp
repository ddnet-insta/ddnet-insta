#include "accounts_worker.h"

bool CAccountsWorker::RconCmdWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
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

	char aBuf[4096];

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
		break;
	case EAccountRconCmd::ACC_LOGOUT:
		return ForceLogout(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_LOCK:
		return LockAccount(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_UNLOCK:
		return UnlockAccount(pSqlServer, pData, pResult, pError, ErrorSize);
	case EAccountRconCmd::ACC_INFO:
		return AccountInfo(pSqlServer, pData, pResult, pError, ErrorSize);
	}

	return false;
}

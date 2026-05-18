#include "mode_account.h"
#include <engine/server/databases/connection.h>
#include <base/dbg.h>
#include <base/log.h>
#include <base/str.h>

bool CAccountTableCity::CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize)
{
	return false;
}

bool CAccountTableCity::Save(IDbConnection *pSqlServer, const char *pUsername, char *pError, int ErrorSize)
{
	char aBuf[1024];
	str_format(
		aBuf,
		sizeof(aBuf),
		"UPDATE %s "
		"SET"
		" level = ? " // TODO: remove hardcode
		"WHERE username = ?;",
		Name());

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare update failed query=%s", aBuf);
		return false;
	}

	pSqlServer->BindInt(1, m_Level); // TODO: remove hardcode
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

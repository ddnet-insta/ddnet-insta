#include "accounts_worker.h"

#include <insta/server/ddnet_db_utils/ddnet_db_utils.h>
#include <insta/server/password_hash.h>

bool CAccountsWorker::CreateAccountsTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	// do not write anything to sqlite3 backup table
	// because it makes no sense for accounts
	// restoring those and merging the data without
	// getting into bad state is super complicated
	if(w != Write::NORMAL)
		return true;

	char aBuf[4096];
	str_format(aBuf, sizeof(aBuf),
		"CREATE TABLE IF NOT EXISTS accounts("
		" id                INTEGER       %s,"
		" username          VARCHAR(%d)   COLLATE %s NOT NULL,"
		" password          VARCHAR(%d)   COLLATE %s NOT NULL,"
		" logged_in         INTEGER       DEFAULT 0,"
		" locked            INTEGER       DEFAULT 0,"
		" server_ip         VARCHAR(64)   NOT NULL DEFAULT '',"
		" server_port       INTEGER       NOT NULL DEFAULT 0,"
		" display_name      VARCHAR(%d)   COLLATE %s NOT NULL DEFAULT '',"
		" contact           VARCHAR(%d)   NOT NULL DEFAULT '',"
		" pin               INTEGER       NOT NULL DEFAULT 0,"
		" register_ip       VARCHAR(64)   NOT NULL,"
		" last_login        TIMESTAMP,"
		" register_date     TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP "
		");",
		ddnet_db_utils::PrimaryKeyAutoIncrement(pSqlServer),
		MAX_USERNAME_LENGTH,
		pSqlServer->BinaryCollate(),
		MAX_HASH_WITH_SALT_LENGTH,
		pSqlServer->BinaryCollate(),
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

bool CAccountsWorker::CreateExtraAccountsTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
		return false;

	const auto *pData = dynamic_cast<const CSqlCreateExtraAccountsTablesRequest *>(pGameData);
	return CExtraAccountTableController::CreateTablesThread(pData->m_vTables, pSqlServer, pError, ErrorSize);
}

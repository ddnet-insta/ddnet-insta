#include "mode_account.h"

#include <base/dbg.h>
#include <base/log.h>
#include <base/str.h>

#include <engine/server/databases/connection.h>

#include <game/server/player.h>

#include <insta/server/account.h>

bool CAccountTableCity::CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_format(aBuf, sizeof(aBuf),
		"CREATE TABLE IF NOT EXISTS account_city("
		" username          VARCHAR(%d)   COLLATE %s NOT NULL,"
		" level             INTEGER       DEFAULT 0,"
		"PRIMARY KEY (username)"
		");",
		MAX_USERNAME_LENGTH,
		pSqlServer->BinaryCollate());

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->Print();
	int NumInserted;
	return pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize);
}

bool CAccountTableCity::Save(IDbConnection *pSqlServer, const char *pUsername, const void *pUserData, char *pError, int ErrorSize)
{
	const CAccountDataCity *pData = static_cast<const CAccountDataCity *>(pUserData);
	const char *pQuery =
		"UPDATE account_city "
		"SET"
		" level = ? " // TODO: remove hardcode
		"WHERE username = ?;";

	if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare update failed query=%s", pQuery);
		return false;
	}

	pSqlServer->BindInt(1, pData->m_Level); // TODO: remove hardcode
	pSqlServer->BindString(2, pUsername);
	pSqlServer->Print();

	int NumUpdated;
	if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
	{
		log_error("sql-thread", "update failed query=%s", pQuery);
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

bool CExtraAccountTableController::Save(class IDbConnection *pSqlServer, const char *pUsername, const CAccount *pAccount, char *pError, int ErrorSize)
{
	bool Ok = true;
	if(pAccount->m_Mode.m_City.has_value())
	{
		log_info("sql-thread", "saving city data...");
		if(!CAccountTableCity::Save(pSqlServer, pAccount->Username(), &pAccount->m_Mode.m_City.value(), pError, ErrorSize))
			Ok = false;
	}
	return Ok;
}

void CExtraAccountTableController::InitPlayer(CPlayer *pPlayer)
{
	for(const auto *pTable : m_vpTables)
	{
		switch (pTable->Type()) {
			case EExtraAccTable::CITY:
				log_info("player", "init city table..");
				pPlayer->m_Account.m_Mode.m_City = CAccountDataCity();
			break;
		}
	}
}

CExtraAccountTableController::~CExtraAccountTableController()
{
	for(auto *pTable : m_vpTables)
	{
		delete pTable;
		pTable = nullptr;
	}
	m_vpTables.clear();
}

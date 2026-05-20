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

bool CAccountTableCity::Load(class IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, char *pError, int ErrorSize)
{
	const char *pQuery =
		"SELECT"
		" level "
		"FROM account_city "
		"WHERE username = ?;";
	if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))
	{
		log_error("sql-thread", "prepare failed query: %s", pQuery);
		return false;
	}
	pSqlServer->BindString(1, pUsername);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		log_error("sql-thread", "step failed query: %s", pQuery);
		return false;
	}

	if(End)
	{
		log_error("sql-thread", "THIS IS BAD");

		// TODO: need to write to pError here i guess
		return false; // not a fatal error but no account loaded
	}

	if(pAccount)
	{
		int Offset = 1;
		pAccount->m_Mode.m_City.m_Level = pSqlServer->GetInt(Offset++);
	}

	return true;
}

bool CAccountTableCity::Save(IDbConnection *pSqlServer, const char *pUsername, const CAccountDataCity *pData, char *pError, int ErrorSize)
{
	// const CAccountDataCity *pData = static_cast<const CAccountDataCity *>(pUserData);
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

bool CExtraAccountTableController::Load(class IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)
{
	bool Ok = true;
	log_info("sql-thread", "loading extra tables..");
	for(const auto Table : vTables)
	{
		switch(Table)
		{
		case EExtraAccTable::CITY:
			log_info("sql-thread", " loading city data...");
			if(!CAccountTableCity::Load(pSqlServer, pUsername, pAccount, pError, ErrorSize))
				Ok = false;
			break;
		}
	}

	if(!Ok)
	{
		log_error("sql-thread", "EXTRA TABLES FAILED TO LOAD");
	}

	return Ok;
}

bool CExtraAccountTableController::Save(class IDbConnection *pSqlServer, const char *pUsername, const CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)
{
	bool Ok = true;

	log_info("sql-thread", "saving extra tables..");

	for(const auto Table : vTables)
	{
		switch(Table)
		{
		case EExtraAccTable::CITY:
			log_info("sql-thread", " saving city data...");
			if(!CAccountTableCity::Save(pSqlServer, pAccount->Username(), &pAccount->m_Mode.m_City, pError, ErrorSize))
				Ok = false;
			break;
		}
	}

	return Ok;
}

void CExtraAccountTableController::InitPlayer(CPlayer *pPlayer)
{
	// TODO: I do not think it is a good idea to init the std optionals here to some empty value
	//       in the sql worker is a bit nasty if we want to load an account and store the result
	//       to a CAccount instance but the load depends on input from a CAccount which is the same
	//       class but different fields used for input and output at the same time
	//       so we end up with
	//       ```C++
	//       CAccount Acc;
	//       CAccount AccExtraInput = pPlayer->m_Account;
	//       LoadAccount(&Acc, &AccExtraInput); // WTF?
	//       ```
	//       Better would be if the sql worker could just enable tables explicitly based on a list of enum values
	//       ```C++
	//       CAccount Acc;
	//       std::vector<EExtraAccTable> vTables;
	//       vTables.emplace_back(EExtraAccTable::CITY);
	//       LoadAccount(&Acc, vTables);
	//       ```
	//       This enum could also be the only identifier the server stores at all.
	//       hm maybe not xd because of create table
	//       we dont even need to store that in the player instance at all we can ask the controller on save
	//       because it is the same for all
	//
	//       i do not like copy pasting a vector around everywhere
	//       so maybe a bit flag or static array would be better
	//       but tbh we copy paste a bunch of strings when loading accounts one smol vector shouldnt have much of an impact

	/*
	for(const auto *pTable : m_vpTables)
	{
		switch (pTable->Type()) {
			case EExtraAccTable::CITY:
				log_info("player", "init city table..");
				pPlayer->m_Account.m_Mode.m_City = CAccountDataCity();
			break;
		}
	}
	*/
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

#pragma once

// TODO: this file should be generated with python

// TODO: two classes? one for player instances and one for the gamemode so it can create the table

#include <base/log.h>

#include <optional>
#include <vector>

class CPlayer;
class CAccount;

enum class EExtraAccTable
{
	CITY,
};

class IAccountTable
{
public:
	virtual ~IAccountTable() = default;

	// name has to start with "account_"
	// virtual const char *Name() const = 0;
	virtual EExtraAccTable Type() const = 0;

	virtual bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) = 0;

	/// pUserData is a instance of CAccountData(name) which will contain the actual data that should be saved
	// virtual bool Save(class IDbConnection *pSqlServer, const char *pUsername, const void *pUserData, char *pError, int ErrorSize) = 0;
};

// TODO: lazy loading would be neat then we need a m_IsLoaded property here
//       then the /login command can finish before all additional tables are queried
//       but that will make the code more complicated everywhere so lets not do it for now
//
//       it still needs a loaded property with the current design
//       because this is stored in a variable that is null when the table
//       is unused and set to empty values when it should be loaded
//       so during the time where this is already set by the mode
//       on player join but the player did not login yet
//       or the query did not load the data yet this will be in unloaded state but non null
class CAccountDataCity
{
public:
	int m_Level = 0;
};

class CAccountTableCity : public IAccountTable
{
public:
	// we need the name in the save method which is static so we have to hardcode it
	// which is fine because the code should be generated anyways
	// const char *Name() const override { return "account_city"; }

	EExtraAccTable Type() const override { return EExtraAccTable::CITY; }

	bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) override;

	static bool Load(class IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, char *pError, int ErrorSize);
	static bool Save(class IDbConnection *pSqlServer, const char *pUsername, const CAccountDataCity *pData, char *pError, int ErrorSize);
};

// player instance
class CModeAccount
{
public:
	// not sure what would be best here
	// a pointer is nice because we can save a bit of memory
	// if a lot of unused tables from other modes exist in the code base
	// but then we have to manage memory and worry about free
	//
	// std::optional would be easier to not mess up memory management
	// but it also always has to allocate all objects even if they are unused in this mode
	//
	// WARNING: don't access this variable directly and instead wrap it in a getter
	//          so the above mentioned refactor can be applied easily
	// std::optional<CAccountDataCity> m_City = std::nullopt; // ok never mind i use an enum vector to request and a value to store

	CAccountDataCity m_City;

	CModeAccount()
	{
		log_info("extra-acc", "MODE ACCOUNT CONSTRUCTED");
	}

	void Reset()
	{
	}
};

// gameserver instance
class CExtraAccountTableController
{
public:
	// TODO: remove this vector and only use the enums
	std::vector<IAccountTable *> m_vpTables;

	std::vector<EExtraAccTable> m_vTables;

	~CExtraAccountTableController();

	static bool Load(class IDbConnection *pSqlServer, const char *pUsername, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize);
	static bool Save(class IDbConnection *pSqlServer, const char *pUsername, const CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize);

	void InitPlayer(CPlayer *pPlayer);
};

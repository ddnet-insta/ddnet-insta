#pragma once

// TODO: this file should be generated with python

// TODO: two classes? one for player instances and one for the gamemode so it can create the table

#include <vector>

class CPlayer;

class IAccountTable
{
	public:
	virtual ~IAccountTable() = default;
	virtual const char *Name() const = 0;
	virtual bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) = 0;
	virtual bool Save(class IDbConnection *pSqlServer, const char *pUsername, char *pError, int ErrorSize) = 0;
};

class CAccountTableCity : public IAccountTable
{
public:
	const char *Name() const override { return "account_city"; }
	bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) override;
	bool Save(class IDbConnection *pSqlServer, const char *pUsername, char *pError, int ErrorSize) override;

	int m_Level = 0;
};

// player instance
class CModeAccount
{
public:
	void Reset()
	{
	}
};

// gameserver instance
class CExtraAccountTableController
{
	public:
		std::vector<IAccountTable *> m_vpTables;

		~CExtraAccountTableController();

	void InitPlayer(CPlayer *pPlayer);
};

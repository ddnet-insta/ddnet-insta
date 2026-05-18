#pragma once

// TODO: this file should be generated with python

// TODO: two classes? one for player instances and one for the gamemode so it can create the table

class IAccountTable
{
	public:
		virtual ~IAccountTable() = default;
	virtual bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) = 0;
	virtual bool Save(class IDbConnection *pSqlServer, const char *pUsername, char *pError, int ErrorSize) = 0;
};

class CAccountTableCity : public IAccountTable
{
public:
	const char *Name() const { return "city"; }
	bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) override;
	bool Save(class IDbConnection *pSqlServer, const char *pUsername, char *pError, int ErrorSize) override;

	int m_Level = 0;
};

class CModeAccount
{
public:
	void Reset()
	{
	}
};

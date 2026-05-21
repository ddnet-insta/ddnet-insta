#!/usr/bin/env python3

from typing import Protocol
import textwrap
import sys
import os
import importlib
import pkgutil
import inspect
from pathlib import Path
from sys import stderr

class SqlCol(Protocol):
    name: str
    data_type: str

class ExtraAccTable(Protocol):
    name: str
    columns: list[SqlCol]

class GenExtraTables:
    def __init__(self):
        self.tables: list[ExtraAccTable] = []
        pass

    def load_tables(self, table_dir: str) -> list[ExtraAccTable]:
        tables = []
        table_path = Path(table_dir)
        for file in table_path.glob("*.py"):
            print(file)

            module_name = file.stem
            class_name = "".join(part.capitalize() for part in module_name.split("_"))
            class_name = "AccTable" + class_name

            # Load the module from file path
            spec = importlib.util.spec_from_file_location(module_name, file)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)

            cls = getattr(module, class_name, None)
            if cls is None:
                print(f"Error: in file {file} the expected class {class_name} was not found!", file=stderr)
                exit(1)
            tables.append(cls)
        return tables

    def header(self):
        code = textwrap.dedent("""
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

            static bool Insert(class IDbConnection *pSqlServer, const char *pUsername, const CAccountDataCity *pData, char *pError, int ErrorSize);
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
        """)
        return code

    def source(self):
        code = textwrap.dedent("""
        #include "mode_account.h"

        #include <base/dbg.h>
        #include <base/log.h>
        #include <base/str.h>

        #include <engine/server/databases/connection.h>

        #include <game/server/player.h>

        #include <insta/server/account.h>

        bool CAccountTableCity::CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize)
        {
            // TODO: make the username a proper unique foreign key

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

        bool CAccountTableCity::Insert(class IDbConnection *pSqlServer, const char *pUsername, const CAccountDataCity *pData, char *pError, int ErrorSize)
        {
            const char *pQuery =
                "INSERT INTO account_city("
                " username, "
                " level "
                ") VALUES ("
                " ?,"
                " ?"
                ");";

            if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))
            {
                log_error("sql-thread", "prepare insert failed query=%s", pQuery);
                return false;
            }

            int Offset = 1;
            pSqlServer->BindString(Offset++, pUsername);
            pSqlServer->BindInt(Offset++, pData->m_Level);
            pSqlServer->Print();

            int NumInserted;
            if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
            {
                return false;
            }

            // TODO: check NumInserted

            return true;
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
                // https://github.com/ddnet-insta/ddnet-insta/pull/660#issuecomment-4496155696
                // the additional data is not guranteed to exist so if we fail to load
                // we assume we have to init it here

                // TODO: do we need to call some proper constructor here?
                //       i feel like this 0 intializes which might not be the defaults
                //       we want for all data
                CAccountDataCity NewCityData = {};

                if(!Insert(pSqlServer, pUsername, &NewCityData, pError, ErrorSize))
                {
                    log_error("sql-thread", "THIS IS BAD");
                    // TODO: need to write to pError here i guess
                    return false;
                }
                pAccount->m_Mode.m_City = NewCityData;
                return true;
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
        """)
        return code

    def print_usage(self):
        print(f"codegen.py [header|source]")

    def cli(self, args: list[str]):
        if len(args) != 2:
            self.print_usage()
            exit(1)
        dir_path = os.path.dirname(os.path.realpath(__file__))
        self.tables = self.load_tables(dir_path + "/acc_tables")
        arg = args[1]
        if arg == 'header':
            print(self.header())
        elif arg == 'source':
            print(self.source())
        elif arg == 'help' or arg == '-h' or arg == '--help':
            self.print_usage()
        else:
            print(f"Invalid arg '{arg}'", file=sys.stderr)
            exit(1)

gen = GenExtraTables()
gen.cli(sys.argv)

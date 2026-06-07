#!/usr/bin/env python3

import textwrap
import sys
import os
import importlib
import pkgutil
import inspect
from pathlib import Path
from sys import stderr

from table import AccTable, name_to_snake, varchar_len

class GenExtraTables:
    def __init__(self):
        self.tables: list[AccTable] = []
        pass

    def load_tables(self, table_dir: str) -> list[AccTable]:
        tables = []
        table_path = Path(table_dir)
        for file in table_path.glob("*.py"):
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
            tables.append(cls())
        return tables

    def table_enum(self) -> str:
        """
        builds a enum that looks something like this

        enum class EExtraAccTable
        {
            CITY,
        };
        """
        lines = [
                "enum class EExtraAccTable",
                "{"
        ]
        for tab in self.tables:
            lines.append(f"    {tab.name_snake().upper()},")
        lines.append("};")
        return "\n".join(lines)

    def data_class(self, table: AccTable) -> str:
        lines = [
            "class CAccountData" + table.name_camel(),
            "{",
            "public:"
        ]

        # fields
        for col in table.columns:
            if col.data_type == "INTEGER":
                default = 0
                if col.default:
                    default = col.default
                var_type = 'int'
                if col.is_int64:
                    var_type = 'int64_t'
                lines.append(f"    {var_type} {col.cpp_name()} = {default};")
            elif col.data_type.startswith('VARCHAR('):
                lines.append(f"    char {col.cpp_name()}[{varchar_len(col.data_type)}];")
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")

        # comparison operator
        lines.append('    bool operator==(const CAccountData' + table.name_camel() + ' &Other) const')
        lines.append('    {')
        lines.append('        return')
        num = 0
        for col in table.columns:
            num += 1
            last = num == len(table.columns)
            field = col.cpp_name()
            and_or_end = ' &&'
            if last:
                and_or_end = ';'
            if col.data_type == "INTEGER":
                lines.append(f"        Other.{field} == {field}{and_or_end}")
            elif col.data_type.startswith('VARCHAR('):
                lines.append(f"        str_comp(Other.{field}, {field}) == 0{and_or_end}")
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")
        lines.append('    }')
        lines.append("};")
        return "\n".join(lines)

    def data_classes(self) -> str:
        """
        defines one header only data class
        for every table they look like this

        class CAccountDataCity
        {
        public:
            int m_Level = 0;
        };
        """
        code = ""
        for tab in self.tables:
            code += self.data_class(tab) + "\n"
        return code

    def behavior_class_header(self, table: AccTable) -> str:
        name = table.name_camel()
        lines = [
        "class CAccountTable" + name + " : public IAccountTable",
        "{",
        "public:",
        "    // we need the name in the save method which is static so we have to hardcode it",
        "    // which is fine because the code should be generated anyways",
        '    // const char *Name() const override { return "account_' + table.name_snake().lower() + '"; }',
        ""
        "    EExtraAccTable Type() const override { return EExtraAccTable::" + table.name_snake().upper() + "; }",
        ""
        "    bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) override;",
        "",
        f"    static bool Insert(class IDbConnection *pSqlServer, int AccountId, const CAccountData{name} *pData, char *pError, int ErrorSize);",
        "    static bool Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, char *pError, int ErrorSize);",
        f"    static bool Save(class IDbConnection *pSqlServer, int AccountId, const CAccountData{name} *pData, bool ExpectChanges, char *pError, int ErrorSize);",
        "};"
        ]
        return "\n".join(lines)

    def behavior_classes_header(self) -> str:
        """
        generates the code for all table classes in the header file
        that define the behavior of interacting with the database
        they look like this

        class CAccountTableCity : public IAccountTable
        {
        public:
            // we need the name in the save method which is static so we have to hardcode it
            // which is fine because the code should be generated anyways
            // const char *Name() const override { return "account_city"; }

            EExtraAccTable Type() const override { return EExtraAccTable::CITY; }

            bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) override;

            static bool Insert(class IDbConnection *pSqlServer, int AccountId, const CAccountDataCity *pData, char *pError, int ErrorSize);
            static bool Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, char *pError, int ErrorSize);
            static bool Save(class IDbConnection *pSqlServer, int AccountId, const CAccountDataCity *pData, char *pError, int ErrorSize);
        };
        """
        code = ""
        for tab in self.tables:
            code += self.behavior_class_header(tab) + "\n"
        return code

    def tables_as_class_members(self) -> str:
        lines = []
        for tab in self.tables:
            name = tab.name_camel()
            lines.append(f"    CAccountData{name} m_{name};")
        lines.append('')
        lines.append('    // WARNING: do not read or write to this field')
        lines.append('    //          this is only used to check if the data changed')
        lines.append('    class')
        lines.append('    {')
        lines.append('    public:')
        for tab in self.tables:
            name = tab.name_camel()
            lines.append(f"        std::optional<CAccountData{name}> m_{name} = std::nullopt;")
        lines.append('    } m_LastKnownDbState;')
        return "\n".join(lines)

    def header(self):
        code = textwrap.dedent("""
        #pragma once

        #include <base/log.h>
        #include <base/str.h>

        #include <optional>
        #include <vector>

        class CPlayer;
        class CAccount;
        """)
        code += self.table_enum()
        code += textwrap.dedent("""
        class IAccountTable
        {
        public:
            virtual ~IAccountTable() = default;

            // name has to start with "account_"
            // virtual const char *Name() const = 0;
            virtual EExtraAccTable Type() const = 0;

            virtual bool CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize) = 0;

            /// pUserData is a instance of CAccountData(name) which will contain the actual data that should be saved
            // virtual bool Save(class IDbConnection *pSqlServer, int AccountId, const void *pUserData, char *pError, int ErrorSize) = 0;
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
        """)
        code += self.data_classes()
        code += self.behavior_classes_header()
        code += textwrap.dedent("""

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
        """)
        code += self.tables_as_class_members()
        code += textwrap.dedent("""
            void Reset()
            {
                // TODO: this is called but does nothing, do we need this?

                // UPDATE: yes I think this is needed for str_copy() for string default values
            }
        };

        // gameserver instance
        class CExtraAccountTableController
        {
        public:
            std::vector<EExtraAccTable> m_vTables;

            ~CExtraAccountTableController();

            static const char *EnumToTableName(EExtraAccTable Table);
            static bool CreateTablesThread(const std::vector<EExtraAccTable> &vTables, IDbConnection *pSqlServer, char *pError, int ErrorSize);
            static bool Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize);
            static bool Save(class IDbConnection *pSqlServer, int AccountId, const CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize);

            void InitPlayer(CPlayer *pPlayer);
        };
        """)
        return code

    def create_table_method(self, table: AccTable) -> str:
        lines = [
        'bool CAccountTable' + table.name_camel() + '::CreateTable(class IDbConnection *pSqlServer, char *pError, int ErrorSize)',
        '{',
        '    char aBuf[4096];',
        '    str_format(aBuf,',
        '        sizeof(aBuf),',
        '        "CREATE TABLE IF NOT EXISTS account_' + table.name_snake().lower() + '("',
        '        " id                              INTEGER       %s,"',
        '        " account_id                      INTEGER       NOT NULL,"'
        ]

        num = 0
        for col in table.columns:
            num += 1
            last = num == len(table.columns)
            name = col.name_snake().lower()
            align = 32
            spaces = " " * (align - len(name))
            comma = ","
            if last:
                comma = ""
            if col.data_type == "INTEGER":
                default = '0'
                if col.default:
                    default = str(col.default)
                lines.append('        " ' + name + spaces + 'INTEGER       DEFAULT ' + default + comma + '"')
            elif col.data_type.startswith('VARCHAR('):
                align2 = 14
                spaces2 = " " * (align2 - len(col.data_type))
                default = ''
                if col.default:
                    default = col.default
                lines.append('        " ' + name + spaces + col.data_type + spaces2 + f"DEFAULT '{default}'" + comma + '"')
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")

        lines += [
        '        ");",',
        '        ddnet_db_utils::PrimaryKeyAutoIncrement(pSqlServer));'
        '',
        '    if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))',
        '    {',
        '        return false;',
        '    }',
        '    pSqlServer->Print();',
        '    int NumInserted;',
        '    if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))',
        '    {',
        '        return false;',
        '    }',
        '',
        '    // apply missing migrations',
        '    // alter the existing table to add new columns if they were added',
        '    // after the database was already filled with data'
        ]

        for col in table.columns:
            if not col.create_if_missing:
                continue

            name = col.name_snake().lower()
            tab_name = "account_" + table.name_snake().lower()
            if col.data_type == "INTEGER":
                default = 0
                if col.default:
                    default = col.default
                lines.append(f"    ddnet_db_utils::AddIntColumn(pSqlServer, \"{tab_name}\", \"{col.sql_name()}\", {default}, pError, ErrorSize);")
            elif col.data_type.startswith('VARCHAR('):
                default = ''
                if col.default:
                    default = col.default
                lines.append(f"    ddnet_db_utils::AddStrColumn(pSqlServer, \"{tab_name}\", \"{col.sql_name()}\", {varchar_len(col.data_type)}, true, true, \"{default}\", pError, ErrorSize);")
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")

        lines += [
        '',
        '    return true;',
        '}'
        ]
        return "\n".join(lines)

    def create_table_methods(self) -> str:
        """
        generates the CreateTable() method implementation
        for all account tables. It looks something like this:

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
        """
        code = ""
        for tab in self.tables:
            code += self.create_table_method(tab) + "\n"
        return code

    def insert_method(self, table: AccTable) -> str:
        lines = [
        'bool CAccountTable' + table.name_camel() + '::Insert(class IDbConnection *pSqlServer, int AccountId, const CAccountData' + table.name_camel() + ' *pData, char *pError, int ErrorSize)'
        '{',
        '    const char *pQuery =',
        '        "INSERT INTO account_' + table.name_snake().lower() + '("',
        '        " account_id, "'
        ]

        num = 0
        for col in table.columns:
            num += 1
            last = num == len(table.columns)
            if last:
                lines.append('        " ' + col.name_snake().lower() + ' "')
            else:
                lines.append('        " ' + col.name_snake().lower() + ', "')

        lines += [
            '        ") VALUES ("',
            '        " ?,"'
        ]

        num = 0
        for _ in table.columns:
            num += 1
            last = num == len(table.columns)
            if last:
                lines.append('        " ?"')
            else:
                lines.append('        " ?,"')

        lines += [
            '        ");";',
            '',
            '    if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))',
            '    {',
            '        log_error("sql-thread", "prepare insert failed query=%s", pQuery);',
            '        return false;',
            '    }'
                '',
            '    int Offset = 1;',
            '    pSqlServer->BindInt(Offset++, AccountId);',
        ]

        for col in table.columns:
            if col.data_type == "INTEGER":
                lines.append('    pSqlServer->BindInt(Offset++, pData->' + col.cpp_name() + ');',)
            elif col.data_type.startswith('VARCHAR('):
                lines.append('    pSqlServer->BindString(Offset++, pData->' + col.cpp_name() + ');',)
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")

        lines += [
            '    pSqlServer->Print();',
            '',
            '    int NumInserted;',
            '    if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))',
            '    {',
            '        return false;',
            '    }',
            '',
            '    // TODO: check NumInserted',
            '',
            '    return true;',
            '}'
        ]
        return "\n".join(lines)

    def insert_methods(self) -> str:
        """
        generates the method implementations for all tables
        that perform the sql insert. looks like this:


        bool CAccountTableCity::Insert(class IDbConnection *pSqlServer, int AccountId, const CAccountDataCity *pData, char *pError, int ErrorSize)
        {
            const char *pQuery =
                "INSERT INTO account_city("
                " account_id, "
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
            pSqlServer->BindInt(Offset++, AccountId);
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
        """
        code = ""
        for tab in self.tables:
            code += self.insert_method(tab) + "\n"
        return code

    def load_method(self, table: AccTable) -> str:
        lines = [
            'bool CAccountTable' + table.name_camel() + '::Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, char *pError, int ErrorSize)',
            '{',
            '    const char *pQuery =',
            '        "SELECT"',
        ]
        num = 0
        for col in table.columns:
            num += 1
            last = num == len(table.columns)
            if last:
                lines.append('        " ' + col.name_snake().lower() + ' "')
            else:
                lines.append('        " ' + col.name_snake().lower() + ', "')
        table_name = f"account_" + table.name_snake().lower()
        lines += [
            '        "FROM ' + table_name + ' "',
            '        "WHERE account_id = ?;";',
            '    if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))',
            '    {',
            '        log_error("sql-thread", "  failed to load extra account table ' + table_name + '");',
            '        log_error("sql-thread", "  sql prepare failed for the following query:");',
            '        log_error("sql-thread", "  %s", pQuery);',
            '        return false;',
            '    }',
            '    pSqlServer->BindInt(1, AccountId);',
            '    pSqlServer->Print();',
            '',
            '    bool End;',
            '    if(!pSqlServer->Step(&End, pError, ErrorSize))',
            '    {',
            '        log_error("sql-thread", "  step failed query: %s", pQuery);',
            '        return false;',
            '    }',
            '',
            '    if(End)',
            '    {',
            '        // https://github.com/ddnet-insta/ddnet-insta/pull/660#issuecomment-4496155696',
            '        // the additional data is not guaranteed to exist so if we fail to load',
            '        // we assume we have to init it here',
            '',
            '        // TODO: do we need to call some proper constructor here?',
            '        //       i feel like this 0 initializes which might not be the defaults',
            '        //       we want for all data',
            '        CAccountData' + table.name_camel() + ' NewData = {};',
            '',
            '        if(!Insert(pSqlServer, AccountId, &NewData, pError, ErrorSize))',
            '        {',
            '            log_error("sql-thread", "THIS IS BAD");',
            '            // TODO: need to write to pError here i guess',
            '            return false;',
            '        }',
            '        pAccount->m_Mode.m_' + table.name_camel() + ' = NewData;',
            '        pAccount->m_Mode.m_LastKnownDbState.m_' + table.name_camel() + ' = NewData;',
            '        return true;',
            '    }',
            '',
            '    if(pAccount)',
            '    {',
            '        int Offset = 1;'
        ]
        for col in table.columns:
            var = f"pAccount->m_Mode.m_{table.name_camel()}.{col.cpp_name()}"
            if col.data_type == "INTEGER" and col.is_int64:
                lines.append(f"        {var} = pSqlServer->GetInt64(Offset++);")
            elif col.data_type == "INTEGER":
                lines.append(f"        {var} = pSqlServer->GetInt(Offset++);")
            elif col.data_type.startswith('VARCHAR('):
                lines.append(f"        pSqlServer->GetString(Offset++, {var}, sizeof({var}));")
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")
        lines.append('')
        lines.append('        pAccount->m_Mode.m_LastKnownDbState.m_' + table.name_camel() + ' = pAccount->m_Mode.m_' + table.name_camel() + ';')
        lines += [
            '    }',
            '',
            '    return true;',
            '}'
        ]
        return "\n".join(lines)

    def load_methods(self) -> str:
        """
        code to load data from db for all tables
        looks like this

        bool CAccountTableCity::Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, char *pError, int ErrorSize)
        {
            const char *pQuery =
                "SELECT"
                " level "
                "FROM account_city "
                "WHERE account_id = ?;";
            if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))
            {
                log_error("sql-thread", "prepare failed query: %s", pQuery);
                return false;
            }
            pSqlServer->BindInt(1, AccountId);
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
                // the additional data is not guaranteed to exist so if we fail to load
                // we assume we have to init it here

                // TODO: do we need to call some proper constructor here?
                //       i feel like this 0 initializes which might not be the defaults
                //       we want for all data
                CAccountDataCity NewData = {};

                if(!Insert(pSqlServer, AccountId, &NewData, pError, ErrorSize))
                {
                    log_error("sql-thread", "THIS IS BAD");
                    // TODO: need to write to pError here i guess
                    return false;
                }
                pAccount->m_Mode.m_City = NewData;
                return true;
            }

            if(pAccount)
            {
                int Offset = 1;
                pAccount->m_Mode.m_City.m_Level = pSqlServer->GetInt(Offset++);
            }

            return true;
        }
        """
        code = ""
        for tab in self.tables:
            code += self.load_method(tab) + "\n"
        return code

    def save_method(self, table: AccTable) -> str:
        lines = [
            'bool CAccountTable' + table.name_camel() + '::Save(IDbConnection *pSqlServer, int AccountId, const CAccountData' + table.name_camel() + ' *pData, bool ExpectChanges, char *pError, int ErrorSize)',
            '{',
            '    // const CAccountDataCity *pData = static_cast<const CAccountDataCity *>(pUserData);',
            '    const char *pQuery =',
            '        "UPDATE account_' +  table.name_snake().lower() + ' "',
            '        "SET"'
        ]
        num = 0
        for col in table.columns:
            num += 1
            last = num == len(table.columns)
            name = col.name_snake().lower()
            if last:
                lines.append('        " ' + name + ' = ? "')
            else:
                lines.append('        " ' + name + ' = ?, "')

        lines += [ 
            '        "WHERE account_id = ?;";',
            '',
            '    if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))',
            '    {',
            '        log_error("sql-thread", "prepare update failed query=%s", pQuery);',
            '        return false;',
            '    }',
            '',
            '    int Offset = 1;',
        ]

        for col in table.columns:
            if col.data_type == "INTEGER":
                lines.append('    pSqlServer->BindInt(Offset++, pData->' + col.cpp_name() + ');')
            elif col.data_type.startswith('VARCHAR('):
                lines.append('    pSqlServer->BindString(Offset++, pData->' + col.cpp_name() + ');')
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")

        lines += [
            '    pSqlServer->BindInt(Offset, AccountId);',
            '    pSqlServer->Print();',
            '',
            '    int NumUpdated;',
            '    if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))',
            '    {',
            '        log_error("sql-thread", "update failed query=%s", pQuery);',
            '        return false;',
            '    }',
            '',
            '    if(g_Config.m_SvDebugAccounts)',
            '        log_info("sql-thread", "saving ' + table.name_camel() + ' detected %s", ExpectChanges ? "changes" : "no changes");',
            '    if(NumUpdated != 1 && ExpectChanges)',
            '    {',
            '        log_error("sql-thread", "affected %d rows when trying to update the account of one player! Tried to insert the same values?", NumUpdated);',
            '        log_error("sql-thread", "variables:");'
            '        log_error("sql-thread", " account_id=%d", AccountId);'
        ]
        for col in table.columns:
            fmt = '%d'
            if col.data_type == "INTEGER" and col.is_int64:
                fmt = '%" PRId64 "'
            elif col.data_type == "INTEGER":
                fmt = '%d'
            elif col.data_type.startswith('VARCHAR('):
                fmt = '%s'
            else:
                raise ValueError(f"in table {table.name_camel()} column {col.name_camel()} has unsupported data type '{col.data_type}'")
            lines.append('        log_error("sql-thread", " ' + col.name_snake().lower() + '=' + fmt + '", pData->' + col.cpp_name() + ');')
        lines += [
            '        log_error("sql-thread", "query:");'
            '        log_error("sql-thread", " %s", pQuery);'
            '        dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");',
            '        return false;',
            '    }',
            '',
            '    return true;',
            '}'
        ]
        return "\n".join(lines)

    def save_methods(self) -> str:
        """
        save methods for all tables
        looks like this

        bool CAccountTableCity::Save(IDbConnection *pSqlServer, int AccountId, const CAccountDataCity *pData, char *pError, int ErrorSize)
        {
            // const CAccountDataCity *pData = static_cast<const CAccountDataCity *>(pUserData);
            const char *pQuery =
                "UPDATE account_city "
                "SET"
                " level = ? "
                "WHERE account_id = ?;";

            if(!pSqlServer->PrepareStatement(pQuery, pError, ErrorSize))
            {
                log_error("sql-thread", "prepare update failed query=%s", pQuery);
                return false;
            }

            pSqlServer->BindInt(1, pData->m_Level);
            pSqlServer->BindInt(2, AccountId);
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
        """
        code = "\n"
        for tab in self.tables:
            code += self.save_method(tab) + "\n"
        return code

    def load_case(self, table: AccTable) -> list[str]:
        """
        builds case in switch statement to load one specific table

        case EExtraAccTable::CITY:
            log_info("sql-thread", " loading city data...");
            if(!CAccountTableCity::Load(pSqlServer, AccountId, pAccount, pError, ErrorSize))
                Ok = false;
            break;
        """
        lines = [
            '        case EExtraAccTable::' + table.name_snake().upper() + ':',
            '            if(g_Config.m_SvDebugAccounts)',
            '                log_info("sql-thread", " loading ' + table.name_camel() + ' data...");',
            '            if(!CAccountTable' + table.name_camel() + '::Load(pSqlServer, AccountId, pAccount, pError, ErrorSize))',
            '            {',
            '                log_error("sql-thread", " account id %d failed to load data from table account_' + table.name_snake().lower() + '", AccountId);',
            '                Errors++;',
            '            }',
            '            break;',
        ]
        return lines

    def controller_load(self):
        """
        generates code that switches over all tables and calls the matching loaders
        looks something like this

        bool CExtraAccountTableController::Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)
        {
            bool Ok = true;
            log_info("sql-thread", "loading extra tables..");
            for(const auto Table : vTables)
            {
                switch(Table)
                {
                case EExtraAccTable::CITY:
                    log_info("sql-thread", " loading city data...");
                    if(!CAccountTableCity::Load(pSqlServer, AccountId, pAccount, pError, ErrorSize))
                        Ok = false;
                    break;
                }
            }

            return Ok;
        }
        """
        lines = [
            'bool CExtraAccountTableController::Load(class IDbConnection *pSqlServer, int AccountId, CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)',
            '{',
            '    int Errors = 0;',
            '    if(g_Config.m_SvDebugAccounts)',
            '        log_info("sql-thread", "loading extra tables..");',
            '    for(const auto Table : vTables)',
            '    {',
            '        switch(Table)',
            '        {',
        ]
        for tab in self.tables:
            lines += self.load_case(tab)
        lines += [
            '        }',
            '    }',
            '',
            '    if(Errors && g_Config.m_SvDebugAccounts)',
            '    {',
            '        log_error("sql-thread", "%d extra account tables failed to load", Errors);',
            '    }',
            '',
            '    return Errors == 0;',
            '}'
        ]
        return "\n".join(lines)

    def save_case(self, table: AccTable) -> list[str]:
        """
        generates case in switch statement that calls save method for one specific table

        case EExtraAccTable::CITY:
            log_info("sql-thread", " saving city data...");
            if(!CAccountTableCity::Save(pSqlServer, pAccount->Id(), &pAccount->m_Mode.m_City, pError, ErrorSize))
                Ok = false;
            break;
        """
        lines = [
            '        case EExtraAccTable::' + table.name_snake().upper() + ':',
            '            {',
            '                if(g_Config.m_SvDebugAccounts)',
            '                    log_info("sql-thread", " saving ' + table.name_camel() + ' data...");',
            '                bool ExpectChanges = false;',
            '                if(pAccount->m_Mode.m_LastKnownDbState.m_' + table.name_camel() + '.has_value())',
            '                {',
            '                    ExpectChanges = pAccount->m_Mode.m_LastKnownDbState.m_' + table.name_camel() + '.value() != pAccount->m_Mode.m_' + table.name_camel() + ';',
            '                }',
            '                if(!CAccountTable' + table.name_camel() + '::Save(pSqlServer, pAccount->Id(), &pAccount->m_Mode.m_' + table.name_camel() + ', ExpectChanges, pError, ErrorSize))',
            '                    Ok = false;',
            '            }',
            '            break;',
        ]
        return lines

    def controller_save(self):
        """
        generates code that switches over all tables and calls the matching savers
        looks something like this

        bool CExtraAccountTableController::Save(class IDbConnection *pSqlServer, int AccountId, const CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)
        {
            bool Ok = true;

            log_info("sql-thread", "saving extra tables..");

            for(const auto Table : vTables)
            {
                switch(Table)
                {
                case EExtraAccTable::CITY:
                    log_info("sql-thread", " saving city data...");
                    if(!CAccountTableCity::Save(pSqlServer, pAccount->Id(), &pAccount->m_Mode.m_City, pError, ErrorSize))
                        Ok = false;
                    break;
                }
            }

            return Ok;
        }
        """

        lines = [
            '',
            'bool CExtraAccountTableController::Save(class IDbConnection *pSqlServer, int AccountId, const CAccount *pAccount, const std::vector<EExtraAccTable> &vTables, char *pError, int ErrorSize)',
            '{',
            '    bool Ok = true;',
            '',
            '    if(g_Config.m_SvDebugAccounts)',
            '        log_info("sql-thread", "saving extra tables..");',
            '',
            '    for(const auto Table : vTables)',
            '    {',
            '        switch(Table)',
            '        {',
        ]
        for tab in self.tables:
            lines += self.save_case(tab)
        lines += [
            '        }',
            '    }',
            '',
            '    return Ok;',
            '}'
        ]
        return "\n".join(lines)

    def create_table_thread_case(self, table: AccTable) -> list[str]:
        lines = [
            '			case EExtraAccTable::' + table.name_snake().upper() + ':',
            '			{',
            '					CAccountTable' + table.name_camel() + ' Table;',
            '					if(!Table.CreateTable(pSqlServer, pError, ErrorSize))',
            '					{',
            '						Ok = false;',
            '					}',
            '			}',
            '			break;'
        ]
        return lines

    def controller_enum_name(self) -> str:
        lines = [
        'const char *CExtraAccountTableController::EnumToTableName(EExtraAccTable Table)'
        '{',
        '    switch(Table)',
        '    {',
        ]
        for tab in self.tables:
            lines.append('        case EExtraAccTable::' + tab.name_snake().upper() + ':')
            lines.append('          return "account_' + tab.name_snake().lower() + '";')
        lines += [
        '    };',
        '    return "error";',
        '}'
        ]
        return "\n".join(lines) + "\n\n"

    def controller_create_thread(self) -> str:
        """
        method called by the worker thread
        which calls the table creation workers for the selected tables
        """
        lines = [
            'bool CExtraAccountTableController::CreateTablesThread(const std::vector<EExtraAccTable> &vTables, IDbConnection *pSqlServer, char *pError, int ErrorSize)',
            '{',
            '	bool Ok = true;',
            '	for(auto TableKind : vTables)',
            '	{',
            '		switch (TableKind) {',
        ]
        for tab in self.tables:
            lines += self.create_table_thread_case(tab)
        lines += [
            '		}',
            '	}',
            '	return Ok;',
            '}'
        ]
        return "\n".join(lines)

    def init_case(self, table: AccTable) -> list[str]:
        return [
            '            case EExtraAccTable::' + table.name_snake().upper() + ':',
            '                log_info("player", "init ' + table.name_camel() + ' table..");',
            '                pPlayer->m_Account.m_Mode.m_' + table.name_camel() + ' = CAccountData' + table.name_camel() + '();',
            '            break;'
        ]

    def init_cases(self) -> str:
        """
        init player tables

        case EExtraAccTable::CITY:
            log_info("player", "init city table..");
            pPlayer->m_Account.m_Mode.m_City = CAccountDataCity();
        break;
        """
        lines = []
        for tab in self.tables:
            lines += self.init_case(tab)
        return "\n".join(lines)

    def source(self):
        code = textwrap.dedent("""
        #include "mode_account.h"

        #include <base/dbg.h>
        #include <base/log.h>
        #include <base/str.h>

        #include <engine/server/databases/connection.h>
        #include <engine/shared/config.h>

        #include <game/server/player.h>

        #include <insta/server/account.h>

        """)
        code += self.create_table_methods()
        code += self.insert_methods()
        code += self.load_methods()
        code += self.save_methods()
        code += self.controller_load()
        code += self.controller_save()
        code += textwrap.dedent("""

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
            for(const auto Table : m_vTables)
            {
                switch (Table) {
        """)
        code += self.init_cases()
        code += textwrap.dedent("""
                }
            }
            */
        }

        CExtraAccountTableController::~CExtraAccountTableController()
        {
            m_vTables.clear();
        }

        """)
        code += self.controller_enum_name()
        code += self.controller_create_thread()
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

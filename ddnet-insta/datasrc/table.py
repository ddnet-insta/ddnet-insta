#!/usr/bin/env python3

import re
from typing import Union

def is_lower_snake_case(text: str) -> bool:
    return re.match('^[a-z][a-z0-9_]*$', text) is not None

def name_to_camel(name_list: list[str]) -> str:
    name = ''.join([part.capitalize() for part in name_list])
    return name

def name_to_snake(name_list: list[str]) -> str:
    name = '_'.join(name_list)
    return name

def split_words(text: str) -> list[str]:
    text = text.replace('_', ' ')
    return re.findall('[A-Z]?[a-z]+|[A-Z]+(?=[A-Z]|$)', text)

def varchar_len(varchar: str) -> int:
    match = re.search(r'VARCHAR\((\d+)\)', varchar)
    if not match:
        return 0
    return int(match.group(1))

class SqlColumn:
    def __init__(
            self,

            # name of the sql column you want to add
            # should be lower_snake_case to follow the projects
            # naming convention
            name: str,

            # the SQL data type used for the column
            # for example "INTEGER" or "VARCHAR(16)"
            data_type: str,

            # if the data_type is "INTEGER"
            # this can be set to true if the C++
            # variable should be int64_t
            is_int64: bool = False,

            # If create missing is set to true it will check if the
            # existing table in the database already has this column
            # if not it will create it on server start to make sure
            # new versions of the code that add new columns still work
            # with older existing databases
            #
            # so you want to set this to true for all columns you added
            # after making a release that already created a database
            #
            # and keep it false if you want to manually apply migrations to your table
            # or if this is the first and final schema of your table
            # it is less sql queries and overhead on server start if this is set to False
            create_if_missing: bool = False,

            # the default value used by the database on insert
            # if this is not set it will use 0 for integers
            # and the empty string for strings
            default: Union[str,int,None] = None,
        ) -> None:
        if not is_lower_snake_case(name):
            raise ValueError(f"column name '{name}' is not valid lower snake case")
        if data_type != 'INTEGER' and not data_type.startswith('VARCHAR('):
            raise ValueError(f"column '{name}' has unknown data_type '{data_type}'")
        if is_int64 and data_type != 'INTEGER':
            raise ValueError(f"column '{name}' is marked as is_int64 but has data_type '{data_type}'")
        if default:
            if data_type == 'INTEGER':
                if not isinstance(default, int):
                    raise ValueError(f"column '{name}' has type INTEGER but the default value is not an integer")
            elif data_type.startswith('VARCHAR('):
                if not isinstance(default, str):
                    raise ValueError(f"column '{name}' has type VARCHAR() but the default value is not a string")

        self.name = split_words(name)
        self.data_type = data_type
        self.is_int64 = is_int64
        self.create_if_missing = create_if_missing
        self.default = default

    def name_snake(self) -> str:
        return name_to_snake(self.name)

    def name_camel(self) -> str:
        return name_to_camel(self.name)

    def cpp_name(self) -> str:
        if self.data_type.startswith('VARCHAR('):
            return f"m_a{self.name_camel()}"
        return f"m_{self.name_camel()}"

    def sql_name(self) -> str:
        return self.name_snake().lower()

class AccTable:
    name: list[str] = []
    columns: list[SqlColumn] = []

    def __init__(self) -> None:
        self.columns = []
        pass

    def name_snake(self) -> str:
        return name_to_snake(self.name)

    def name_camel(self) -> str:
        return name_to_camel(self.name)



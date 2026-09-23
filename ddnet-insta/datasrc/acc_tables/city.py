#!/usr/bin/env python3

from table import AccTable, SqlColumn

class AccTableCity(AccTable):
    def __init__(self) -> None:
        self.columns.append(SqlColumn("coins", "INTEGER"))

        # if you later want to add more columns you can do it like this
        # self.columns.append(SqlColumn("my_new_field", "INTEGER", create_if_missing=True, default=42))

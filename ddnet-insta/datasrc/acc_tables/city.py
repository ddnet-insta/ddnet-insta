#!/usr/bin/env python3

from table import AccTable, SqlColumn

class AccTableCity(AccTable):
    def __init__(self) -> None:
        super().__init__("city")
        self.columns.append(SqlColumn("level", "INTEGER"))
        self.columns.append(SqlColumn("money", "INTEGER"))
        self.columns.append(SqlColumn("xp", "INTEGER"))
        self.columns.append(SqlColumn("health", "INTEGER"))
        self.columns.append(SqlColumn("foo", "INTEGER", create_if_missing=True))
        self.columns.append(SqlColumn("gaming", "INTEGER", create_if_missing=True))
        self.columns.append(SqlColumn("yellow", "INTEGER", create_if_missing=True))
        self.columns.append(SqlColumn("wood", "INTEGER", create_if_missing=True))
        self.columns.append(SqlColumn("wood slap", "INTEGER", create_if_missing=True))
        self.columns.append(SqlColumn("name", "VARCHAR(16)", create_if_missing=True))

#!/usr/bin/env python3

from table import AccTable, SqlColumn

class AccTableCity(AccTable):
    def __init__(self) -> None:
        super().__init__("city")
        self.columns = []
        self.columns.append(SqlColumn("level", "INTEGER"))
        self.columns.append(SqlColumn("money", "INTEGER"))
        self.columns.append(SqlColumn("xp", "INTEGER"))
        self.columns.append(SqlColumn("health", "INTEGER"))

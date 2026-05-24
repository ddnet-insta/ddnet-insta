#!/usr/bin/env python3

from table import AccTable, SqlColumn

class AccTableCity(AccTable):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.columns.append(SqlColumn("level", "INTEGER"))


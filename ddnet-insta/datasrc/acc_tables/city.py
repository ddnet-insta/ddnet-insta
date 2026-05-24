#!/usr/bin/env python3

from table import AccTable, SqlColumn

class AccTableCity(AccTable):
    def __init__(self) -> None:
        super().__init__("city")
        self.columns.append(SqlColumn("level", "INTEGER"))

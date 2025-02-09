#!/usr/bin/env python3

from table import AccTable, SqlColumn

class AccTableMmo(AccTable):
    def __init__(self) -> None:
        super().__init__("mmo")
        self.columns.append(SqlColumn("coins", "INTEGER"))
        self.columns.append(SqlColumn("stones", "INTEGER"))

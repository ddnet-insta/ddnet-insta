#!/usr/bin/env python3

from typing import Protocol

class SqlCol(Protocol):
    name: str
    data_type: str

class ExtraAccTable(Protocol):
    name: str
    columns: list[SqlCol]


class GenExtraTables:
    def __init__(self):
        pass

    def generate(self, acc_table: ExtraAccTable):
        print("foo")

class AccTableCity:
    name = "foo"
    columns = []
    pass

gen = GenExtraTables()
gen.generate(AccTableCity())

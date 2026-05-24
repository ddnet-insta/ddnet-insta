#!/usr/bin/env python3

import re

def name_to_camel(name_list: list[str]) -> str:
    name = ''.join([part.capitalize() for part in name_list])
    return name

def name_to_snake(name_list: list[str]) -> str:
    name = '_'.join(name_list)
    return name

def split_words(text: str) -> list[str]:
    text = text.replace('_', ' ')
    return re.findall('[A-Z]?[a-z]+|[A-Z]+(?=[A-Z]|$)', text)

class SqlColumn:
    def __init__(self, name: str, data_type: str) -> None:
        self.name = split_words(name)
        self.data_type = data_type
        pass

    def name_snake(self) -> str:
        return name_to_snake(self.name)

    def name_camel(self) -> str:
        return name_to_camel(self.name)


class AccTable:
    name: list[str] = []
    columns: list[SqlColumn] = []

    def __init__(self, name: str) -> None:
        self.name = split_words(name)
        pass

    def name_snake(self) -> str:
        return name_to_snake(self.name)

    def name_camel(self) -> str:
        return name_to_camel(self.name)


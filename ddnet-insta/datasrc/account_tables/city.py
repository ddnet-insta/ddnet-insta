#!/usr/bin/env python3

import sys
from codegen import GenExtraTables

class AccTableCity:
    name = "foo"
    columns = []
    pass

gen = GenExtraTables()
gen.cli(sys.argv, AccTableCity())

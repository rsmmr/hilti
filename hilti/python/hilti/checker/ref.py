# $Id$
#
# Ref checks.

from hilti.instructions import ref
from hilti.core import *
from checker import checker

import sys

@checker.when(ref.Assign)
def _(self, i):
    if i.op1().type() != i.target().type():
        self.error(i, "operand type does not match target's type")



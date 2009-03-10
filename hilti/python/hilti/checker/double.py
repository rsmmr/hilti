# $Id$
#
# Double checks.

from hilti.instructions import double
from hilti.core import *
from checker import checker

@checker.when(double.Div)
def _(self, i):
    if isinstance(i.op2(), instruction.ConstOperand) and i.op2().value() == 0:
        self.error(i, "division by zero")

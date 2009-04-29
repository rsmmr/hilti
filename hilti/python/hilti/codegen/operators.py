# $Id$
#
# Code generator for operators.

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.when(instruction.Operator)
def _(self, i):
    self.llvmExecuteOperator(i)

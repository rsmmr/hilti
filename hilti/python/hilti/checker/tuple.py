# $Id$
#
# Tuple checks.

from hilti.instructions import tuple
from hilti.core import *
from checker import checker

import sys

@checker.when(tuple.Index)
def _(self, i):
    
    if not isinstance(i.op2(), instruction.ConstOperand):
        self.error(i, "tuple index must be constant")
        return
    
    types = i.op1().type().types()
    idx = i.op2().value()

    if idx < 0 or idx >= len(types):
        self.error(i, "tuple index out of range")
        return
    
    if types[idx] != i.target().type():
        self.error(i, "target's type does not match tuple index %d" % idx)

@checker.when(tuple.Assign)
def _(self, i):
    if not i.op1().type() == i.target().type():
        self.error(i, "types of target and operand 1 do not match")
    
    
    


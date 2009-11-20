# $Id$
# 
# Checking operators. 

from hilti.instructions import flow
from hilti.core import *
from checker import checker

def _checkOverloadedOperator(i):
    ops = instruction.findOverloadedOperator(i)
    
    if len(ops) == 0:
        print i.target().type(), i.op1().type(), i.op2().type()
        checker.error(i, "no matching implementation of overloaded operator found")
    
    if len(ops) > 1:
        checker.error(i,  "use of overloaded operator is ambigious, multiple matching implementations found:")
        
        for o in ops:
            checker.error(i, "%s" % o._signature, indent=True)

@checker.when(instruction.Operator)
def _(self, i):
    _checkOverloadedOperator(i)
        


# $Id$
# 
# Checking operators. 

from hilti.instructions import flow
from hilti.core import *
from checker import checker

@checker.when(instruction.Operator)
def _(self, i):
    (num, ovop) = instruction.findOverloadedOperator(i)
    
    if num == 0:
        checker.error(i, "no matching implementation of overloaded operator found")
        return;
    
    if num > 1:
        checker.error(i, "use of overloaded operator is ambigious, multiple matching implementations found")
        return
        


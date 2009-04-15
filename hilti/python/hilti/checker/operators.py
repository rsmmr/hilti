# $Id$
# 
# Checking operators. 

from hilti.instructions import flow
from hilti.core import *
from checker import checker
from hilti import codegen

@checker.when(instruction.Operator)
def _(self, i):
    # Relying on the code generator here is not great but that's the only
    # place where the information is available.
    
    t = i.operatorType()
    
    if not codegen.codegen.codegen.implementsOperator(t, i):
        checker.error(i, "type %s does not implement operator %s" % (t, i.name()))


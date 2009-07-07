# $Id$
#
# Code generator for debug instructions. 

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.when(instructions.debug.Msg)
def _(self, i):
    if codegen.debugMode():
        self.llvmGenerateCCallByName("hlt::debug_printf", [i.op1(), i.op2(), i.op3()])
    

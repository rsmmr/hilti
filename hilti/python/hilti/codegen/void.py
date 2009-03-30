# $Id$ 
#
# Code generation for the void type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.llvmType(type.Void)
def _(type, refine_to):
    return llvm.core.Type.void()




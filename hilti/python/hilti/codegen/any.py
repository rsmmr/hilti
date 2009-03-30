# $Id$ 
#
# Code generation for the any type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.llvmType(type.Any)
def _(type, refine_to):
    return llvm.core.Type.pointer(llvm.core.Type.int(8))


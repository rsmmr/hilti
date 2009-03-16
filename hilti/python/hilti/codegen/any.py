# $Id$ 
#
# Code generation for the any type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertTypeToLLVM(type.Any)
def _(type, refine_to):
    return llvm.core.Type.pointer(llvm.core.Type.int(8))

@codegen.convertTypeToC(type.Any)
def _(type, refine_to):
    return codegen.llvmTypeConvert(type, refine_to)

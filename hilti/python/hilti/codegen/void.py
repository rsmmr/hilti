# $Id$ 
#
# Code generation for the void type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertTypeToLLVM(type.Void)
def _(type, refine_to):
    return llvm.core.Type.void()

@codegen.convertTypeToC(type.Void)
def _(type, refine_to):
    return codegen.llvmTypeConvert(type, refine_to)

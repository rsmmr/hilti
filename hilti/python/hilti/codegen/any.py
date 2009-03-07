# $Id$ 
#
# Code generation for the any type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertTypeToLLVM(type.Any)
def _(type):
    return llvm.core.Type.pointer(llvm.core.Type.int(8))

@codegen.convertTypeToC(type.Any)
def _(type):
    return codegen.convertTypeToLLVM(type)

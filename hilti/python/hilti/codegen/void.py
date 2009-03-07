# $Id$ 
#
# Code generation for the void type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.convertTypeToLLVM(type.Void)
def _(type):
    return llvm.core.Type.void()

@codegen.convertTypeToC(type.Void)
def _(type):
    return codegen.convertTypeToLLVM(type)

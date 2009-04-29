# $Id$ 
#
# Code generation for the meta type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.llvmType(type.MetaType)
def _(type, refine_to):
    return llvm.core.Type.pointer(codegen.llvmTypeTypeInfo())

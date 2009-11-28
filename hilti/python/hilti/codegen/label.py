# $Id$
#
# Code generator for label instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
TODO.
"""

@codegen.typeInfo(type.Label)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "void *"
    return typeinfo

@codegen.llvmDefaultValue(type.Label)
def _(type):
    return llvm.core.Constant.null(codegen.llvmTypeConvert(type.refType()))

@codegen.llvmType(type.Label)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()


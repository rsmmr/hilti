# $Id$
#
# Code generator for the continuation type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
An ``continuation`` is mapped to an ``hlt_continuation *``.
"""

@codegen.typeInfo(type.Continuation)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_continuation *"
    return typeinfo

@codegen.llvmType(type.Continuation)
def _(type, refine_to):
    return llvm.core.Type.pointer(codegen.llvmTypeContinuation())


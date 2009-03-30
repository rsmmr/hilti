# $Id$
#
# Code generator for bool instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``bool`` is mapped to an ``int8_t``, with ``True`` corresponding to the
value ``1`` and ``False`` to value ``0``.
"""

@codegen.typeInfo(type.Bool)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "int8_t"
    typeinfo.to_string = "__Hlt::bool_to_string";
    typeinfo.to_int64 = "__Hlt::bool_to_int64";
    return typeinfo

@codegen.llvmDefaultValue(type.Bool)
def _(type):
    return codegen.llvmConstInt(0, 1)

@codegen.llvmCtorExpr(type.Bool)
def _(op, refine_to):
    return codegen.llvmConstInt(1 if op.value() else 0, 1)

@codegen.llvmType(type.Bool)
def _(type, refine_to):
    return llvm.core.Type.int(1)


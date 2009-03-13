# $Id$
#
# Code generator for bool instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.Bool)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::bool_to_string";
    typeinfo.to_int64 = "__Hlt::bool_to_int64";
    return typeinfo

@codegen.convertCtorExprToLLVM(type.Bool)
def _(op):
    return llvm.core.Constant.int(llvm.core.Type.int(1), 1 if op.value() else 0)

@codegen.convertTypeToLLVM(type.Bool)
def _(type):
    return llvm.core.Type.int(1)

@codegen.convertTypeToC(type.Bool)
def _(type):
    """A ``bool`` is mapped to an ``int8_t``, with ``True`` corresponding to
    the value ``1`` and ``False`` to value ``0``."""
    return codegen.llvmTypeConvert(type)

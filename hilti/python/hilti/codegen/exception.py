# $Id$
#
# Code generator for the exception type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
An ``exception`` is mapped to an ``hlt_exception *``.
"""

@codegen.typeInfo(type.Exception)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_exception *"
    return typeinfo

@codegen.llvmType(type.Exception)
def _(type, refine_to):
    return codegen.llvmTypeExceptionPtr()

@codegen.operator(instructions.exception.New)
def _(self, op):
    arg = codegen.llvmOp(op.op2()) if op.op2() else None
    excpt = codegen.llvmNewException(op.op1().value(), arg, op.location())
    codegen.llvmStoreInTarget(op.target(), excpt)

@codegen.when(instructions.exception.Throw)
def _(self, i):
    exception = codegen.llvmOp(i.op1())
    codegen.llvmRaiseException(exception)
    
    
    

    
    
    


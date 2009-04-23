# $Id$ 
#
# Code generation for the ref type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``ref<T>`` is mapped to the same type as ``T``. Note that because ``T`` must
be a ~~HeapType, and all ~~HeapTypes are mapped to pointers, ``ref<T>`` will
likewise be mapped to a pointer. In addition, the type information for
``ref<T>`` will be the type information of ``T``.
"""

@codegen.typeInfo(type.Reference)
def _(type):
    # We only create a type info for the wildcard type as that's the only one we
    # need.
    if type.wildcardType():
        typeinfo = codegen.TypeInfo(type)
        typeinfo.c_prototype = "void *"
        return typeinfo

    return None

@codegen.llvmDefaultValue(type.Reference)
def _(type):
    return llvm.core.Constant.null(codegen.llvmTypeConvert(type.refType()))

@codegen.llvmCtorExpr(type.Reference)
def _(op, refine_to):
    # This can only be "None".
    if op.value() == None:
        if refine_to and op.type().wildcardType():
            return llvm.core.Constant.null(codegen.llvmTypeConvert(refine_to))
        
        return llvm.core.Constant.null(codegen.llvmTypeGenericPointer())
        
    else:
        # Pass on to ctor of referenced type.
        #
        # FIXME: Write a function for doing the callback.
        return codegen._callCallback(codegen._CB_CTOR_EXPR, op.type().refType(), [op, refine_to])

@codegen.llvmType(type.Reference)
def _(type, refine_to):
    if refine_to and type.wildcardType():
        return llvm.core.Type.pointer(refine_to)

    if type.wildcardType():
        # We need some type so just call it an integer pointer.
        return llvm.core.Type.pointer(llvm.core.Type.int(8))
    
    return codegen.llvmTypeConvert(type.refType())

@codegen.when(instructions.ref.CastBool)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    voidp = codegen.builder().bitcast(op1, codegen.llvmTypeGenericPointer())
    null = llvm.core.Constant.null(codegen.llvmTypeGenericPointer())
    result = codegen.builder().icmp(llvm.core.IPRED_EQ, voidp, null)
    codegen.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.ref.Assign)
def _(self, i):
    op1 = self.llvmOp(i.op1(), i.target().type())
    codegen.llvmStoreInTarget(i.target(), op1)




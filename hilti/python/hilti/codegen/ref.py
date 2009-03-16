# $Id$ 
#
# Code generation for the ref type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.Reference)
def _(type):
    # No type info for refs.
    return None

@codegen.convertCtorExprToLLVM(type.Reference)
def _(op, refine_to):
    # This can only be "None".
    assert op.value() == None
    
    if refine_to and op.type().wildcardType():
        return llvm.core.Constant.pointer(refine_to, 0)
        
    return llvm.core.Constant.pointer(codegen.llvmTypeGenericPointer(), 0)

@codegen.convertTypeToLLVM(type.Reference)
def _(type, refine_to):
    if refine_to and op.type().wildcardType():
        return llvm.core.Type.pointer(refine_to)

    return codegen.llvmTypeGenericPointer()
        
@codegen.convertTypeToC(type.Reference)
def _(type, refine_to):
    """A ``ref<T>`` is mapped to the same type as ``T``. Note that because
    ``T`` must be a ~~HeapType, and all ~~HeapTypes are mapped to pointers,
    ``ref<T>`` will likewise be mapped to a pointer. Furthermore, the type
    information passed ``ref<T>`` will also be that of ``T``.
    """
    return codegen.llvmTypeConvert(type, refine_to)

@codegen.when(instructions.ref.CastBool)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    voidp = codegen.builder().bitcast(op1, llvmTypeGenericPointer())
    null = llvm.core.Constant.pointer(codegen.llvmTypeGenericPointer(), 0)
    return codegen.builder().icmp(IPRED_EQ, voidp, null)




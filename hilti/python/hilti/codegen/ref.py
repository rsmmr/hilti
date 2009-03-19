# $Id$ 
#
# Code generation for the ref type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.Reference)
def _(type):
    # No type information.
    return None

@codegen.defaultInitValue(type.Reference)
def _(type):
    return llvm.core.Constant.null(codegen.llvmTypeConvert(type.refType()))

@codegen.convertCtorExprToLLVM(type.Reference)
def _(op, refine_to):
    # This can only be "None".
    assert op.value() == None
    
    if refine_to and op.type().wildcardType():
        return llvm.core.Constant.null(codegen.llvmTypeConvert(refine_to))
        
    return llvm.core.Constant.null(codegen.llvmTypeGenericPointer())

@codegen.convertTypeToLLVM(type.Reference)
def _(type, refine_to):
    if refine_to and op.type().wildcardType():
        return llvm.core.Type.pointer(refine_to)

    return codegen.llvmTypeConvert(type.refType())
        
@codegen.convertTypeToC(type.Reference)
def _(type, refine_to):
    """A ``ref<T>`` is mapped to the same type as ``T``. Note that because
    ``T`` must be a ~~HeapType, and all ~~HeapTypes are mapped to pointers,
    ``ref<T>`` will likewise be mapped to a pointer. In addition, the type
    information for ``ref<T>`` will be the type information of ``T``.
    """
    return codegen.llvmTypeConvertToC(type.refType())

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




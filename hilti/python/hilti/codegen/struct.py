# $Id$ 
#
# Code generation for the struct type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.Struct)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    return typeinfo

@codegen.convertCtorExprToLLVM(type.Struct)
def _(op, refine_to):
    assert False

@codegen.convertTypeToLLVM(type.Struct)
def _(type, refine_to):
    structt = [llvm.core.Type.int(32)] + [codegen.llvmTypeConvert(id.type()) for (id, default) in type.Fields()]
    return llvm.core.Type.pointer(structt)
        
@codegen.convertTypeToC(type.Struct)
def _(type, refine_to):
    """
    A ``struct` is passed as a pointer to an eqivalent C struct; the fields'
    types are converted recursively per the same rules. 
    """
    structt = [llvm.core.Type.int(32)] + [codegen.llvmTypeConvertToC(id.type()) for (id, default) in type.Fields()]
    return llvm.core.Type.pointer(structt)

@codegen.when(instructions.struct.New)
def _(self, i):
    pass

@codegen.when(instructions.struct.Get)
def _(self, i):
    pass

@codegen.when(instructions.struct.Set)
def _(self, i):
    pass

@codegen.when(instructions.struct.Unset)
def _(self, i):
    pass


#    op1 = self.llvmOp(i.op1())
#    voidp = codegen.builder().bitcast(op1, llvmTypeGenericPointer())
#    null = llvm.core.Constant.pointer(codegen.llvmTypeGenericPointer(), 0)
#    return codegen.builder().icmp(IPRED_EQ, voidp, null)






# $Id$
#
# Code generator for tuple instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

import sys

@codegen.makeTypeInfo(type.Tuple)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::tuple_to_string";
    return typeinfo

def _tupleType(type):
    llvm_types = [codegen.llvmTypeConvert(t) for t in type.types()]
    return llvm.core.Type.struct(llvm_types)

# FIXME: rename to ValToLLVM
@codegen.convertConstToLLVM(type.Tuple)
def _(op):
    t = _tupleType(op.type())
    struct = codegen.builder().alloca(t)
    
    length = len(op.type().types())
    vals = op.value()
    
    for i in range(length):
        dst = codegen.builder().gep(struct, [codegen.llvmConstInt(0), codegen.llvmConstInt(i)])
        codegen.builder().store(codegen.llvmOp(vals[i]), dst)

    return codegen.builder().load(struct)
    
@codegen.convertTypeToLLVM(type.Tuple)
def _(type):
    return _tupleType(type)

@codegen.convertTypeToC(type.Tuple)
def _(type):
    """A ``tuple<type1,type2,...>`` is mapped to C by passing a pointer to an
    *array of pointers* to the individual elements. 
    """
    return llvm.core.Type.pointer(llvm.core.Type.array(codegen.llvmTypeGenericPointer(), 0))

@codegen.convertValueToC(type.Tuple)
def _(ll, type):
    
    length = len(type.types())
    
    # Need to turn struct value into addr. Hope this can be optimized away
    # usually. FIXME: I'm sure there's a better way to do this ...
    addr = codegen.builder().alloca(ll.type)
    codegen.builder().store(ll, addr)
    
    ptrs = [codegen.builder().gep(addr, [codegen.llvmConstInt(0), codegen.llvmConstInt(i)]) for i in range(length)]
    ptrs = [codegen.builder().bitcast(ptr, codegen.llvmTypeGenericPointer()) for ptr in ptrs]
    
    arrayt = llvm.core.Type.array(codegen.llvmTypeGenericPointer(), length)
    array = codegen.builder().alloca(arrayt)
    for i in range(length):
        dst = codegen.builder().gep(array, [codegen.llvmConstInt(0), codegen.llvmConstInt(i)])
        codegen.builder().store(ptrs[i], dst)

    return array

@codegen.when(instructions.tuple.Assign)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    self.llvmStoreInTarget(i.target(), op1)

@codegen.when(instructions.tuple.Index)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
 
    # This is way ugly. Is there something better to get an addr for gep from a
    # value?
    addr = codegen.builder().alloca(op1.type)
    codegen.builder().store(op1, addr)
    ptr = self.builder().gep(addr, [codegen.llvmConstInt(0), op2], "XXX")
    result = self.builder().load(ptr)
    self.llvmStoreInTarget(i.target(), result)



# $Id$ 
#
# Code generation for the struct type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """ 
A ``struct` is passed as a pointer to an eqivalent C struct; the fields' types
are converted recursively per the same rules. 
"""

def _llvmStructType(struct):
    # Currently, we support only up to 32 fields.
    assert len(struct.Fields()) <= 32
    fields = [llvm.core.Type.int(32)] + [codegen.llvmTypeConvert(id.type()) for (id, default) in struct.Fields()]
    return llvm.core.Type.struct(fields)

@codegen.typeInfo(type.Struct)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    return typeinfo

@codegen.llvmCtorExpr(type.Struct)
def _(op, refine_to):
    assert False

@codegen.llvmType(type.Struct)
def _(type, refine_to):
    return llvm.core.Type.pointer(_llvmStructType(type))
       
@codegen.when(instructions.struct.New)
def _(self, i):
    # Allocate memory for struct. 
    structt = i.op1().value()
    llvm_type = _llvmStructType(structt)
    s = codegen.llvmMalloc(llvm_type)
    
    # Initialize fields
    zero = codegen.llvmGEPIdx(0)
    mask = 0
    
    fields = structt.Fields()
    for j in range(len(fields)):
        (id, default) = fields[j]
        if default:
            # Initialize with default. 
            mask |= (1 << j)
            index = codegen.llvmGEPIdx(j + 1)
            addr = codegen.builder().gep(s, [zero, index])
            codegen.llvmInit(codegen.llvmOp(default, id.type()), addr)
        else:
            # Leave untouched. As we keep the bitmask of which fields are
            # set,  we will never access it. 
            pass
        
    # Set mask.
    addr = codegen.builder().gep(s, [zero, zero])
    codegen.llvmInit(codegen.llvmConstInt(mask, 32), addr)

    codegen.llvmStoreInTarget(i.target(), s)
    
def _getIndex(instr):
    
    fields = instr.op1().type().refType().Fields()
    
    for i in range(len(fields)):
        (id, default) = fields[i]
        if id.name() == instr.op2().value():
            return (i, id.type())
        
    return (-1, None)

@codegen.when(instructions.struct.Get)
def _(self, i):
    (idx, ftype) = _getIndex(i)
    assert idx >= 0
    
    s = codegen.llvmOp(i.op1())
    
    # Check whether field is valid. 
    zero = codegen.llvmGEPIdx(0)
    
    addr = codegen.builder().gep(s, [zero, zero])
    mask = codegen.builder().load(addr)
    
    bit = codegen.llvmConstInt(1<<idx, 32)
    isset = codegen.builder().and_(bit, mask)
    
    block_ok = self.llvmNewBlock("ok")
    block_exc = self.llvmNewBlock("exc")
    
    notzero = self.builder().icmp(llvm.core.IPRED_NE, isset, self.llvmConstInt(0, 32))
    self.builder().cbranch(notzero, block_ok, block_exc)
    
    self.pushBuilder(block_exc)
    self.llvmRaiseExceptionByName("__hlt_exception_undefined_value")
    self.popBuilder()
    
    self.pushBuilder(block_ok)
    
    # Load field.
    index = codegen.llvmGEPIdx(idx + 1)
    addr = codegen.builder().gep(s, [zero, index])
    codegen.llvmStoreInTarget(i.target(), codegen.builder().load(addr))
    
@codegen.when(instructions.struct.Set)
def _(self, i):
    (idx, ftype) = _getIndex(i)
    assert idx >= 0

    s = codegen.llvmOp(i.op1())
    
    # Set mask bit.
    zero = codegen.llvmGEPIdx(0)
    addr = codegen.builder().gep(s, [zero, zero])
    mask = codegen.builder().load(addr)
    bit = codegen.llvmConstInt(1<<idx, 32)
    mask = codegen.builder().or_(bit, mask)
    codegen.llvmAssign(mask, addr)
    
    index = codegen.llvmGEPIdx(idx + 1)
    addr = codegen.builder().gep(s, [zero, index])
    codegen.llvmAssign(codegen.llvmOp(i.op3(), ftype), addr)

@codegen.when(instructions.struct.Unset)
def _(self, i):
    (idx, ftype) = _getIndex(i)
    assert idx >= 0

    s = codegen.llvmOp(i.op1())
    
    # Clear mask bit.
    zero = codegen.llvmGEPIdx(0)
    addr = codegen.builder().gep(s, [zero, zero])
    mask = codegen.builder().load(addr)
    bit = codegen.llvmConstInt(~(1<<idx), 32)
    mask = codegen.builder().and_(bit, mask)
    codegen.llvmAssign(mask, addr)

@codegen.when(instructions.struct.IsSet)
def _(self, i):
    (idx, ftype) = _getIndex(i)
    assert idx >= 0

    s = codegen.llvmOp(i.op1())

    # Check mask.
    zero = codegen.llvmGEPIdx(0)
    addr = codegen.builder().gep(s, [zero, zero])
    mask = codegen.builder().load(addr)
    
    bit = codegen.llvmConstInt(1<<idx, 32)
    isset = codegen.builder().and_(bit, mask)

    notzero = self.builder().icmp(llvm.core.IPRED_NE, isset, self.llvmConstInt(0, 32))
    codegen.llvmStoreInTarget(i.target(), notzero)


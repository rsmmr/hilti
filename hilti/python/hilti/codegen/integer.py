# $Id$
#
# Code generator for integer instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

import system

_doc_c_conversion = """
An ``int<n>`` is mapped to C integers depending on its width *n*, per the
following table: 
    
    ======  =======
    Width   C type
    ------  -------
    1..8    int8_t
    9..16   int16_t
    17..32  int32_t
    33..64  int64_t
    ======  =======
"""

@codegen.typeInfo(type.Integer)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::int_to_string";
    typeinfo.to_int64 = "__Hlt::int_to_int64";
    
    for (w, p) in [(8, "int8_t"), (16, "int16_t"), (32, "int32_t"), (64, "int64_t")]:
        if type.width() <= w:
            typeinfo.c_prototype = p
            break
    else:
        assert False
    
    return typeinfo

@codegen.llvmDefaultValue(type.Integer)
def _(type):
    return codegen.llvmConstInt(0, type.width())

@codegen.llvmCtorExpr(type.Integer)
def _(op, refine_to):
    width = op.type().width()
    if not width and refine_to and refine_to.width():
        width = refine_to.width()
        
    return codegen.llvmConstInt(op.value(), width)
    
@codegen.llvmType(type.Integer)
def _(type, refine_to):
    return llvm.core.Type.int(type.width())

@codegen.operator(instructions.integer.Incr)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmConstInt(1, i.op1().type().width())
    result = self.builder().add(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(instructions.integer.Equal)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Add)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().add(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Sub)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().sub(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Mul)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    
    result = self.builder().mul(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Div)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())

    block_ok = self.llvmNewBlock("ok")
    block_exc = self.llvmNewBlock("exc")

    iszero = self.builder().icmp(llvm.core.IPRED_NE, op2, self.llvmConstInt(0, i.op2().type().width()))
    self.builder().cbranch(iszero, block_ok, block_exc)
    
    self.pushBuilder(block_exc)
    self.llvmRaiseExceptionByName("__hlt_exception_division_by_zero") 
    self.popBuilder()
    
    self.pushBuilder(block_ok)
    result = self.builder().sdiv(op1, op2)
    addr = self.llvmAddrLocalVar(self.currentFunction(), self.llvmCurrentFramePtr(), i.target().id().name())
    self.llvmAssign(result, addr)
    self.llvmStoreInTarget(i.target(), result)

    # Leave ok-builder for subsequent code. 
    
@codegen.when(instructions.integer.Eq)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.integer.Lt)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_SLT, op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.integer.Ext)
def _(self, i):
    width = i.target().type().width()
    assert width >= i.op1().type().width()
    
    op1 = self.llvmOp(i.op1())
    
    result = self.builder().zext(op1, self.llvmTypeConvert(type.Integer(width)))
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.integer.Trunc)
def _(self, i):
    width = i.target().type().width()
    assert width <= i.op1().type().width()
    
    op1 = self.llvmOp(i.op1())
    
    result = self.builder().trunc(op1, self.llvmTypeConvert(type.Integer(width)))
    self.llvmStoreInTarget(i.target(), result)
        

### Unpack operator. This is a bit cumbersome due to the many options.

_Unpacks = {   
    "Int8Little":  (0,  8, 0, [0]),
    "Int16Little": (1, 16, 0, [0, 1]),
    "Int32Little": (2, 32, 0, [0, 1, 2, 3]),
    "Int64Little": (3, 64, 0, [0, 1, 2, 3, 4, 5, 6, 7]),
    
    "Int8Big":  (4,  8, 0, [0]),
    "Int16Big": (5, 16, 0, [1, 0]),
    "Int32Big": (6, 32, 0, [3, 2, 1, 0]),
    "Int64Big": (7, 64, 0, [7, 6, 5, 4, 3, 2, 1, 0]),
    
    "UInt8Little":  (8,  8, 16, [0]),
    "UInt16Little": (9, 16, 32, [0, 1]),
    "UInt32Little": (10, 32, 64, [0, 1, 2, 3]),
#   "UInt64Little": (11, 64, 128, [0, 1, 2, 3, 4, 5, 6, 7]),
    
    "UInt8Big":  (12,  8, 16, [0]),
    "UInt16Big": (13, 16, 32, [1, 0]),
    "UInt32Big": (14, 32, 64, [3, 2, 1, 0]),
#   "UInt64Big": (15, 64, 128, [7, 6, 5, 4, 3, 2, 1]),
}

suffix = "Little" if system.isLittleEndian() else "Big"

# Add aliases for host byte order. 
for (key, val) in _Unpacks.items():
    if key.endswith(suffix):
        # Strip suffix.
        _Unpacks[key[0:-len(suffix)]] = key

# Generate the unpack code for a single case.        
def _makeUnpack(cg, label, value, op, end, target, force_width = 0):
    
    start = cg.llvmOp(op)
    end = cg.llvmOp(end)
    
    # FIXME: We don't check the end position yet.
    (id, width, extend, bytes) = value
    last = bytes[-1]

    itype = llvm.core.Type.int(width)
    result = llvm.core.Constant.null(itype)
    builder = cg.builder()

    # Copy the iterator.
    iter = builder.alloca(start.type)
    builder.store(start, iter)
    iterdst = builder.bitcast(iter, cg.llvmTypeGenericPointer())
    
    # Function is defined in hilti_intern.ll
    extract_one = cg.llvmCurrentModule().get_function_named("__hlt_bytes_extract_one")
    exception = cg.llvmAddrException(cg.llvmCurrentFramePtr())

    for i in range(len(bytes)):
        byte = builder.call(extract_one, [iterdst, end, exception])
        byte.calling_convention = llvm.core.CC_C
        byte = builder.sext(byte, itype)
        if bytes[i]:
            byte = builder.shl(byte, cg.llvmConstInt(bytes[i] * 8, width))
        result = builder.or_(result, byte)

    if extend:
        result = builder.zext(result, llvm.core.Type.int(extend))

    if force_width: 
        result = builder.sext(result, llvm.core.Type.int(force_width))
        
    # It's fine to check for an exception at the end rather than after each call.
    cg._llvmGenerateExceptionTest(exception)
    
    iter = cg.builder().load(iter) # Update builder.
    
    cg.llvmStoreTupleInTarget(target, (result, iter))

@codegen.operator(instructions.integer.Unpack)
def _(self, i):
    builder = self.builder()
    
    # We differentiate two cases: constant vs. variable format specifiier.
    if isinstance(i.op3(), instruction.ConstOperand):
        # Constant version.
        try:
            label = i.op3().value()
            value = _Unpacks[i.op3().value()]
            
            if isinstance(value, str):
                # Dereference the alias.
                label = value
                value = _Unpacks[value]
                
            (id, width, extend, bytes) = value
            itype = llvm.core.Type.int(width)
            _makeUnpack(self, label, value, i.op1(), i.op2(), i.target())
            
        except KeyError:
            util.internal_error("instructions.integer.Unpack: unknown format %s" % i.op3().value())

    else:
        # Non-constant version.
        #
        # Note that because we don't track constants across HILTI programs, we
        # may arrive here even if working on a constant. In that case, the
        # optimizer will hopefully remove all but one case. 
        
        default = self.llvmNewBlock("switch-default")
        self.pushBuilder(default) 
        self.llvmRaiseExceptionByName("__hlt_exception_value_error")
        self.popBuilder() 
        
        packed = self.currentModule().lookupID("Hilti::Packed")
        assert packed
        
        builder = self.builder()
        
        labels = packed.type().declType().labels()
        cont = self.llvmNewBlock("after-switch")
        blocks = {}
        cases = []
    
        switch = builder.switch(self.llvmOp(i.op3()), default)
        
        for (label, value) in _Unpacks.items():
            
            # Get the integer corresponding to the label.
            idx = labels[label]
            assert idx
            
            if isinstance(value, str):
                # Dereference the alias.
                label = value
                value = _Unpacks[value]

            (id, width, extend, bytes) = value
            if id in blocks:
                block = blocks[id]
            else:
                block = self.llvmNewBlock(label)
                self.pushBuilder(block)
                _makeUnpack(self, label, value, i.op1(), i.op2(), i.target(), force_width=64)
                self.builder().branch(cont)
                self.popBuilder()
                
                blocks[id] = block
            
            switch.add_case(llvm.core.Constant.int(llvm.core.Type.int(8), idx), block)
            
        self.pushBuilder(cont) # Leave on stack.
            

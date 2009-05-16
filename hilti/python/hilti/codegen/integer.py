# $Id$
#
# Code generator for integer instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

import system

import sys

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

@codegen.when(instructions.integer.Mod)
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
    result = self.builder().srem(op1, op2)
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
        
_Unpacks = {   
    "Hilti::Packed::Int8Little": (0,  8, 0, [0]),
    "Hilti::Packed::Int16Little": (1, 16, 0, [0, 1]),
    "Hilti::Packed::Int32Little": (2, 32, 0, [0, 1, 2, 3]),
    "Hilti::Packed::Int64Little": (3, 64, 0, [0, 1, 2, 3, 4, 5, 6, 7]),
    
    "Hilti::Packed::Int8Big":  (4,  8, 0, [0]),
    "Hilti::Packed::Int16Big": (5, 16, 0, [1, 0]),
    "Hilti::Packed::Int32Big": (6, 32, 0, [3, 2, 1, 0]),
    "Hilti::Packed::Int64Big": (7, 64, 0, [7, 6, 5, 4, 3, 2, 1, 0]),
    
    "Hilti::Packed::UInt8Little":  (8,  8, 16, [0]),
    "Hilti::Packed::UInt16Little": (9, 16, 32, [0, 1]),
    "Hilti::Packed::UInt32Little": (10, 32, 64, [0, 1, 2, 3]),
#   "Hilti::Packed::UInt64Little": (11, 64, 128, [0, 1, 2, 3, 4, 5, 6, 7]),
    
    "Hilti::Packed::UInt8Big":  (12,  8, 16, [0]),
    "Hilti::Packed::UInt16Big": (13, 16, 32, [1, 0]),
    "Hilti::Packed::UInt32Big": (14, 32, 64, [3, 2, 1, 0]),
#   "Hilti::Packed::UInt64Big": (15, 64, 128, [7, 6, 5, 4, 3, 2, 1]),
}

_localSuffix = "Little" if system.isLittleEndian() else "Big"

@codegen.unpack(type.Integer)
def _(t, begin, end, fmt, arg):
    """Integer unpacking supports the following formats:
    
    .. literalinclude:: /libhilti/hilti.hlt
       :start-after: %doc-packed-int-start
       :end-before:  %doc-packed-int-end

    Integer unpacking behaves slightly different depenidng whether the
    ``Hilti::Packed`` format is given as a constant or not. If it's not a
    constant, the unpacked integer will always have a width of 64 bits,
    independent of what kind of integer is stored in the binary data. If it is
    a constant, then for the signed variants (i.e., ``Hilti::Packed::Int*``),
    the width of the target integer corresponds to the width of the unpacked
    integer. For the unsigned variants (i.e.,``Hilti::Packet::UInt*``), the
    width of the target integer must be "one step" *larger* than that of the
    unpacked value: for ``Packed::Uint8`` is must be ``int16`, for
    ``Packed::UInt8`` is must be ``int32`, etc. This is because we don't have
    *unsigned* integers in HILTI.

    Optionally, an additional arguments may be given, which then must be a
    ``tuple<int<8>,int<8>``. If given, the tuple specifies a bit range too
    extract from the unpacked value. For example, ``(6,9)`` extracts the
    bits 6-9. The extraced bits are shifted to the right so that they align at
    bit 0 and the result is returned (with the same width as it would have had
    without extracting any subset of bits).
    """
    
    val = codegen.llvmAlloca(codegen.llvmTypeConvert(t))
    iter = codegen.llvmAlloca(codegen.llvmTypeConvert(type.IteratorBytes()))

    # Generate the unpack code for a single case.        
    def unpackOne(spec):
        def _unpackOne(case):
            # FIXME: We don't check the end position yet.
            (id, width, extend, bytes) = spec
            
            builder = codegen.builder()
            itype = llvm.core.Type.int(width)
            result = llvm.core.Constant.null(itype)
        
            # Copy the iterator.
            builder.store(begin, iter)
            iter_casted = builder.bitcast(iter, codegen.llvmTypeGenericPointer())
            
            # Function is defined in hilti_intern.ll
            extract_one = codegen.llvmCurrentModule().get_function_named("__hlt_bytes_extract_one")
            exception = codegen.llvmAddrException(codegen.llvmCurrentFramePtr())
        
            for i in range(len(bytes)):
                byte = builder.call(extract_one, [iter_casted, end, exception])
                byte.calling_convention = llvm.core.CC_C
                byte = builder.zext(byte, itype)
                if bytes[i]:
                    byte = builder.shl(byte, codegen.llvmConstInt(bytes[i] * 8, width))
                result = builder.or_(result, byte)
        
            if extend:
                result = builder.zext(result, llvm.core.Type.int(extend))
                width = extend
        
            if t.width() > width: 
                result = builder.sext(result, llvm.core.Type.int(t.width()))
                
            # It's fine to check for an exception at the end rather than after each call.
            codegen._llvmGenerateExceptionTest(exception)

            if arg:
                builder = codegen.builder()
                
                # Extract bits. Fortunately, LLVM has an intrinsic for that. 
                low = codegen.llvmExtractValue(arg, 0)
                high = codegen.llvmExtractValue(arg, 1)
                
                def normalize(i):
                    # FIXME: Ideally, these int should come in here in a
                    # well-defined width. Currently, they don't ...
                    if i.type.width < width:
                        i = codegen.builder().zext(low, llvm.core.Type.int(width))
                    if i.type.width > width:
                        i = codegen.builder().trunc(high, llvm.core.Type.int(width))
                    return i

                low = normalize(low)    
                high = normalize(high)    
                
                # Well, it has not:
                #
                # Assertion failed: (0 && "part_select intrinsic not
                # implemented"), function visitIntrinsicCall, file
                # SelectionDAGBuild.cpp, line 3787.
                #
                # result = codegen.llvmCallIntrinsic(llvm.core.INTR_PART_SELECT, [result.type], [result, low, high])
                #
                # FIXME: So we build it ourselves until LLVM provides it.
                result = builder.lshr(result, low)
                bits = builder.sub(codegen.llvmConstInt(width, width), high)
                bits = builder.add(bits, low)
                bits = builder.sub(bits, codegen.llvmConstInt(1, width))
                mask = builder.lshr(codegen.llvmConstInt(-1, width), bits)
                result = builder.and_(result, mask)
                
            codegen.llvmInit(result, val)
        
        return _unpackOne

    def makeIdx(key):
        if not key.endswith(_localSuffix):
            return key
        
        return [key, key[0:-len(_localSuffix)]]
    
    cases = [(makeIdx(key), unpackOne(spec)) for (key, spec) in _Unpacks.items()]
    codegen.llvmSwitch(fmt, cases)
    return (codegen.builder().load(val), codegen.builder().load(iter))


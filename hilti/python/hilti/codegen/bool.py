# $Id$
#
# Code generator for bool instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``bool`` is mapped to an ``int8_t``, with ``True`` corresponding to the
value ``1`` and ``False`` to value ``0``.
"""

@codegen.typeInfo(type.Bool)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "int8_t"
    typeinfo.to_string = "hlt::bool_to_string";
    typeinfo.to_int64 = "hlt::bool_to_int64";
    return typeinfo

@codegen.llvmDefaultValue(type.Bool)
def _(type):
    return codegen.llvmConstInt(0, 1)

@codegen.llvmCtorExpr(type.Bool)
def _(op, refine_to):
    return codegen.llvmConstInt(1 if op.value() else 0, 1)

@codegen.llvmType(type.Bool)
def _(type, refine_to):
    return llvm.core.Type.int(1)

@codegen.unpack(type.Bool)
def _(t, begin, end, fmt, arg):
    """Bool unpacking uses the format ``Hilti::Packed::Bool``. It reads a
    single byte and, per default, returns ``True` if that byte is non-zero and
    ``False`` otherwise. Optionally, one can specify a particular bit (0-7) as
    additional ``unpack`` arguments and the result will then reflect the value
    of that bit. 
    """

    # FIXME: Add error checking. We don't check the format at the moment nor
    # the range of the bit number. 
    
    addr = codegen.llvmAlloca(codegen.llvmTypeConvert(t))
    iter = codegen.llvmAlloca(codegen.llvmTypeConvert(type.IteratorBytes(type.Bytes())))

    (val, i) = codegen.llvmUnpack(type.Integer(8), begin, end, "Hilti::Packed::Int8")

    builder = codegen.builder()
    
    if arg:
        if arg.type.width > 8:
            arg = builder.trunc(arg, llvm.core.Type.int(8))
        
        mask = builder.shl(codegen.llvmConstInt(1, 8), arg)
        val = builder.and_(val, mask)
        
    result = builder.icmp(llvm.core.IPRED_NE, codegen.llvmConstInt(0, 8), val)
        
    return (result, i)

@codegen.when(instructions.bool.And)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().and_(op1, op2)
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.bool.Or)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().or_(op1, op2)
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.bool.Not)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    result = self.builder().xor(op1, codegen.llvmConstInt(1, 1))
    self.llvmStoreInTarget(i.target(), result)
    

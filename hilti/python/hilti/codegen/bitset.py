# $Id$
#
# Code generator for bitset instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``bitset`` is mapped to an ``int64_t``, with one bit in that value
corresponding to each defined label.

An ``bitset``'s type information keeps additional information in the ``aux``
field: ``aux`` points to a concatenation of ASCIIZ strings containing the
label names followed by the bit number.
"""

@codegen.typeInfo(type.Bitset)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "int64_t"
    typeinfo.to_string = "hlt::bitset_to_string";
    typeinfo.to_int64 = "hlt::bitset_to_int64";
    
    # Build the ASCIIZ strings.
    aux = []
    zero = [codegen.llvmConstInt(0, 8)]
    for (label, value) in sorted(type.labels().items(), key=lambda x: x[1]):
        aux += [codegen.llvmConstInt(ord(c), 8) for c in label] + zero

    name = codegen.nameTypeInfo(type) + "_labels"
    const = llvm.core.Constant.array(llvm.core.Type.int(8), aux)
    glob = codegen.llvmCurrentModule().add_global_variable(const.type, name)
    glob.global_constant = True    
    glob.initializer = const
    glob.linkage = llvm.core.LINKAGE_LINKONCE
    
    typeinfo.aux = glob
    
    return typeinfo

@codegen.llvmDefaultValue(type.Bitset)
def _(type):
    return codegen.llvmConstInt(0, 64)

@codegen.llvmType(type.Bitset)
def _(type, refine_to):
    return llvm.core.Type.int(64)

@codegen.llvmCtorExpr(type.Bitset)
def _(op, refine_to):
    return codegen.llvmConstInt(1, 64).shl(codegen.llvmConstInt(op.value(), 64))

@codegen.when(instructions.bitset.Equal)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bitset.Set)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().or_(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bitset.Clear)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    op2 = self.builder().xor(op2, codegen.llvmConstInt(-1, 64))
    result = self.builder().and_(op1, op2)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bitset.Has)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    tmp = self.builder().and_(op1, op2)
    result = self.builder().icmp(llvm.core.IPRED_EQ, tmp, op2)
    self.llvmStoreInTarget(i.target(), result)

# $Id$
#
# Code generator for tuple instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``tuple<type1,type2,...>`` is mapped to a C struct that consists of fields of the
corresponding type. For example, a ``tuple<int32, string>`` is mapped to a
``struct { int32_t f1, __hlt_string* f2 }``.

A tuple's type-information keeps additional layout information in the ``aux``
field: ``aux`` points to an array of ``int16_t``, one for each field. Each
array entry gives the offset of the corresponding field from the start of the
struct. This is useful for C functions that work with tuples of arbitrary
types as they otherwise would not have any portable way of addressing the
individual fields. 
"""

import sys

def _tupleType(type, refine_to):
    llvm_types = [codegen.llvmTypeConvert(t) for t in type.types()]
    return llvm.core.Type.struct(llvm_types)

@codegen.typeInfo(type.Tuple)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::tuple_to_string";
    
    # Calculate the offset array. 
    zero = codegen.llvmGEPIdx(0)
    null = llvm.core.Constant.null(llvm.core.Type.pointer(_tupleType(type, None)))
    
    offsets = []
    for i in range(len(type.types())):
        idx = codegen.llvmGEPIdx(i)
        # This is a pretty awful hack but I can't find a nicer way to
        # calculate the offsets as *constants*, and this hack is actually also
        # used by LLVM internaly to do sizeof() for constants so it can't be
        # totally disgusting. :-)
        offset = null.gep([zero, idx]).ptrtoint(llvm.core.Type.int(16))
        offsets += [offset]

    name = codegen.nameTypeInfo(type) + "_offsets"
    const = llvm.core.Constant.array(llvm.core.Type.int(16), offsets)
    glob = codegen.llvmCurrentModule().add_global_variable(const.type, name)
    glob.global_constant = True    
    glob.initializer = const

    typeinfo.aux = glob
    
    return typeinfo

@codegen.llvmDefaultValue(type.Tuple)
def _(type):
    t = _tupleType(type, None)
    consts = [codegen.llvmConstDefaultValue(t) for t in type.types()]
    return llvm.core.Constant.struct(consts)

@codegen.llvmCtorExpr(type.Tuple)
def _(op, refine_to):
    t = _tupleType(op.type(), refine_to)
    struct = codegen.llvmAlloca(t)
    
    length = len(op.type().types())
    vals = op.value()
    
    for i in range(length):
        dst = codegen.builder().gep(struct, [codegen.llvmGEPIdx(0), codegen.llvmGEPIdx(i)])
        codegen.builder().store(codegen.llvmOp(vals[i]), dst)

    return codegen.builder().load(struct)
    
@codegen.llvmType(type.Tuple)
def _(type, refine_to):
    return _tupleType(type, refine_to)

@codegen.when(instructions.tuple.Index)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())

    result = self.llvmExtractValue(op1, op2)
    self.llvmStoreInTarget(i.target(), result)



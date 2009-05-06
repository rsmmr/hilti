# $Id$
#
# Code generator for enum instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
An ``enum`` is mapped to an ``int8_t``, with a unique integer corresponding to
each label.

An ``enum``'s type information keep additional information in the ``aux``
field: ``aux`` points to a concatenation of ASCIIZ strings containing the
label names, starting with the undefined value and ordered in the order of
their corresponding integer values.
"""

@codegen.typeInfo(type.Enum)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "int8_t"
    typeinfo.to_string = "__Hlt::enum_to_string";
    typeinfo.to_int64 = "__Hlt::enum_to_int64";
    
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
    
    typeinfo.aux = glob
    
    return typeinfo

@codegen.llvmDefaultValue(type.Enum)
def _(type):
    return codegen.llvmConstInt(type.labels()["Undef"], 8)

@codegen.llvmType(type.Enum)
def _(type, refine_to):
    return llvm.core.Type.int(8)

@codegen.llvmCtorExpr(type.Enum)
def _(op, refine_to):
    value = op.type().labels()[op.value()]
    return codegen.llvmConstInt(value, 8)

@codegen.operator(instructions.enum.Equal)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.builder().icmp(llvm.core.IPRED_EQ, op1, op2)
    self.llvmStoreInTarget(i.target(), result)
    

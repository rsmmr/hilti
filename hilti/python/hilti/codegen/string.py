# $Id$
#
# Code generator for string instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.makeTypeInfo(type.String)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.libhilti_fmt = "__hlt_string_fmt";
    return typeinfo

def _llvmStringType(len=0):
    return llvm.core.Type.packed_struct([llvm.core.Type.int(32), llvm.core.Type.array(llvm.core.Type.int(8), len)])

def _llvmStringTypePtr(len=0):
    return llvm.core.Type.pointer(_llvmStringType(len))

@codegen.convertConstToLLVM(type.String)
def _(op, cast_to):
    assert not cast_to or cast_to == op.type()

    s = op.value().encode("utf-8")
    size = llvm.core.Constant.int(llvm.core.Type.int(32), len(s))
    bytes = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in s]
    struct = llvm.core.Constant.packed_struct([size, llvm.core.Constant.array(llvm.core.Type.int(8), bytes)])
        
    name = codegen.nameNewConstant("string")
    glob = codegen.llvmCurrentModule().add_global_variable(_llvmStringType(len(s)), name)
    glob.global_constant = True
    glob.initializer = struct
    glob.linkage = llvm.core.LINKAGE_INTERNAL
        
    # We need to cast the const, which has a specific array length, to the
    # actual string type, which has unspecified array length. 
    return codegen.builder().bitcast(glob, _llvmStringTypePtr(), name)

@codegen.convertTypeToLLVM(type.String)
def _(type):
    return _llvmStringTypePtr()

@codegen.when(instructions.string.Assign)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    self.llvmStoreInTarget(i.target(), op1)
    
@codegen.when(instructions.string.Length)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    len = self.llvmGenerateLibHiltiCall("__hlt_string_len", [op1])
    self.llvmStoreInTarget(i.target(), len)

@codegen.when(instructions.string.Concat)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    op2 = self.llvmOp(i.op2())
    result = self.llvmGenerateLibHiltiCall("__hlt_string_concat", [op1, op2])
    self.llvmStoreInTarget(i.target(), result)

    

    




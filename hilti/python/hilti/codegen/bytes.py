# $Id$
#
# Code generator for Bytes instructions.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """A ``bytes`` object is mapped to ``__Hlt::bytes *``. The
type is defined in |hilti_intern.h|."""

@codegen.typeInfo(type.Bytes)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__Hlt::bytes *"
    typeinfo.to_string = "__Hlt::bytes_to_string"
    return typeinfo

@codegen.typeInfo(type.IteratorBytes)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__Hlt::bytes_pos *"
    return typeinfo

@codegen.llvmType(type.Bytes)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.llvmType(type.IteratorBytes)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.llvmDefaultValue(type.IteratorBytes)
def _(type):
    return llvm.core.Constant.null(codegen.llvmTypeGenericPointer())

@codegen.operator(type.Bytes, instructions.operators.New)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_new", [], [])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Assign)
def _(self, i):
    op1 = self.llvmOp(i.op1())
    self.llvmStoreInTarget(i.target(), op1)
    
@codegen.when(instructions.bytes.Length)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_len", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Empty)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_empty", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Append)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_append", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])

@codegen.when(instructions.bytes.Sub)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_sub", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Offset)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_offset", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.Begin)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_begin", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.bytes.End)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::bytes_end", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)
    
    




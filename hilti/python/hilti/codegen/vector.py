# $Id$ 
#
# Code generation for the vector type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``vector`` is mapped to a ``hlt_vector *``.
"""

@codegen.typeInfo(type.Vector)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_vector *"
    return typeinfo

@codegen.llvmType(type.Vector)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.llvmType(type.IteratorVector)
def _(type, refine_to):
    # struct hlt_vector_iter
    return llvm.core.Type.struct([codegen.llvmTypeGenericPointer(), llvm.core.Type.int(64)])

@codegen.typeInfo(type.IteratorVector)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_vector_iter"
    return typeinfo

@codegen.llvmDefaultValue(type.IteratorVector)
def _(type):
    # Must match with what the C implementation expects as end() marker.
    return llvm.core.Constant.struct([llvm.core.Constant.null(codegen.llvmTypeGenericPointer()), codegen.llvmConstInt(0, 64)])

@codegen.operator(instructions.vector.New)
def _(self, i):
    t = i.op1().value().itemType()
    result = self.llvmGenerateCCallByName("hlt::vector_new", [self.llvmConstDefaultValue(t)], [t], llvm_args=True)
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.vector.Get)
def _(self, i):
    t = i.op1().type().refType().itemType()
    voidp = self.llvmGenerateCCallByName("hlt::vector_get", [i.op1(), i.op2()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))

@codegen.when(instructions.vector.Set)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::vector_set", [i.op1(), i.op2(), i.op3()])

@codegen.when(instructions.vector.Size)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::vector_size", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.vector.Reserve)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::vector_reserve", [i.op1(), i.op2()])

@codegen.when(instructions.vector.Begin)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::vector_begin", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.vector.End)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::vector_end", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.vector.IterIncr)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::vector_iter_incr", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(instructions.vector.IterDeref)
def _(self, i):
    t = i.target().type()
    voidp = self.llvmGenerateCCallByName("hlt::vector_iter_deref", [i.op1()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))
    
@codegen.operator(instructions.vector.IterEqual)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::vector_iter_eq", [i.op1(), i.op2()])
    self.llvmStoreInTarget(i.target(), result)

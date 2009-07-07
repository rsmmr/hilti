# $Id$
#
# Code generation for the list type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``list`` is mapped to a ``hlt_list *``.
"""

@codegen.typeInfo(type.List)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_list *"
    typeinfo.to_string = "hlt::list_to_string"
    return typeinfo

@codegen.llvmType(type.List)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()

@codegen.llvmType(type.IteratorList)
def _(type, refine_to):
    # struct hlt_list_iter
    return llvm.core.Type.struct([codegen.llvmTypeGenericPointer()] * 2)

@codegen.typeInfo(type.IteratorList)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "hlt_list_iter"
    return typeinfo

@codegen.llvmDefaultValue(type.IteratorList)
def _(type):
    # Must match with what the C implementation expects as end() marker.
    return llvm.core.Constant.struct([llvm.core.Constant.null(codegen.llvmTypeGenericPointer())] * 2)

@codegen.operator(instructions.list.New)
def _(self, i):
    t = instruction.TypeOperand(i.op1().value().itemType())
    result = self.llvmGenerateCCallByName("hlt::list_new", [t])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.list.PushFront)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::list_push_front", [i.op1(), i.op2()])

@codegen.when(instructions.list.PushBack)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::list_push_back", [i.op1(), i.op2()])

@codegen.when(instructions.list.PopFront)
def _(self, i):
    t = i.op1().type().refType().itemType()
    voidp = self.llvmGenerateCCallByName("hlt::list_pop_front", [i.op1()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))

@codegen.when(instructions.list.PopBack)
def _(self, i):
    t = i.op1().type().refType().itemType()
    voidp = self.llvmGenerateCCallByName("hlt::list_pop_back", [i.op1()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))

@codegen.when(instructions.list.Front)
def _(self, i):
    t = i.op1().type().refType().itemType()
    voidp = self.llvmGenerateCCallByName("hlt::list_front", [i.op1()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))

@codegen.when(instructions.list.Back)
def _(self, i):
    t = i.op1().type().refType().itemType()
    voidp = self.llvmGenerateCCallByName("hlt::list_back", [i.op1()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))

@codegen.when(instructions.list.Size)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::list_size", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.list.Erase)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::list_erase", [i.op1()])

@codegen.when(instructions.list.Insert)
def _(self, i):
    self.llvmGenerateCCallByName("hlt::list_insert", [i.op1(), i.op2()])
    
@codegen.when(instructions.list.Begin)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::list_begin", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.when(instructions.list.End)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::list_end", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)
    
@codegen.operator(instructions.list.IterIncr)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::list_iter_incr", [i.op1()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.operator(instructions.list.IterDeref)
def _(self, i):
    t = i.target().type()
    voidp = self.llvmGenerateCCallByName("hlt::list_iter_deref", [i.op1()])
    casted = codegen.builder().bitcast(voidp, llvm.core.Type.pointer(codegen.llvmTypeConvert(t)))
    self.llvmStoreInTarget(i.target(), self.builder().load(casted))
    
@codegen.operator(instructions.list.IterEqual)
def _(self, i):
    result = self.llvmGenerateCCallByName("hlt::list_iter_eq", [i.op1(), i.op2()])
    self.llvmStoreInTarget(i.target(), result)



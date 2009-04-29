# $Id$ 
#
# Code generation for the channel type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = """
A ``channel`` is mapped to a ``__hlt_channel *``. The type is defined in
|hilti_intern.h|:

.. literalinclude:: /libhilti/hilti_intern.h
   :start-after: %doc-hlt_channel-start
   :end-before:  %doc-hlt_channel-end
   
"""

@codegen.typeInfo(type.Channel)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "__Hlt::channel *"
    typeinfo.to_string = "__Hlt::channel_to_string";
    return typeinfo

@codegen.llvmCtorExpr(type.Channel)
def _(op, refine_to):
    assert False

@codegen.llvmType(type.Channel)
def _(type, refine_to):
    return codegen.llvmTypeGenericPointer()
       
@codegen.operator(instructions.channel.New)
def _(self, i):
    top = instruction.TypeOperand(i.op1().value())
    result = self.llvmGenerateCCallByName("__Hlt::channel_new", [top], [top.type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.channel.Write)
def _(self, i):
    self.llvmGenerateCCallByName("__Hlt::channel_try_write", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])

@codegen.when(instructions.channel.Read)
def _(self, i):
    voidp = self.llvmGenerateCCallByName("__Hlt::channel_try_read", [i.op1()], [i.op1().type()])
    item_type = i.op1().type().refType().itemType()
    nodep = self.builder().bitcast(voidp, llvm.core.Type.pointer(self.llvmTypeConvert(item_type)))
    self.llvmStoreInTarget(i.target(), self.builder().load(nodep))

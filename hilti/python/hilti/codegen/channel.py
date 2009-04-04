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

def _llvmChannelType():
    return llvm.core.Type.pointer(llvm.core.Type.int(8))

@codegen.typeInfo(type.Channel)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.c_prototype = "const __hlt_channel *"
    typeinfo.to_string = "__Hlt::channel_to_string";
    return typeinfo

@codegen.llvmCtorExpr(type.Channel)
def _(op, refine_to):
    assert False

@codegen.llvmType(type.Channel)
def _(type, refine_to):
    return _llvmChannelType()
       
@codegen.when(instructions.channel.New)
def _(self, i):
    result = self.llvmGenerateCCallByName("__Hlt::channel_new", [i.op1()], [i.op1().type()])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.channel.Write)
def _(self, i):
    self.llvmGenerateCCallByName("__Hlt::channel_write", [i.op1(), i.op2()], [i.op1().type(), i.op2().type()])

@codegen.when(instructions.channel.Read)
def _(self, i):
    voidp = self.llvmGenerateCCallByName("__Hlt::channel_read", [i.op1()], [i.op1().type()])
    ch_type = i.op1().type().refType().channelType()
    nodep = self.builder().bitcast(voidp, llvm.core.Type.pointer(self.llvmTypeConvert(ch_type)))
    self.llvmStoreInTarget(i.target(), self.builder().load(nodep))

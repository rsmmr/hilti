# $Id$ 
#
# Code generation for the channel type.

import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

def _llvmChannelType():
    return llvm.core.Type.pointer(llvm.core.Type.int(8))

@codegen.makeTypeInfo(type.Channel)
def _(type):
    typeinfo = codegen.TypeInfo(type)
    typeinfo.to_string = "__Hlt::channel_to_string";
    return typeinfo

@codegen.convertCtorExprToLLVM(type.Channel)
def _(op, refine_to):
    assert False

@codegen.convertTypeToLLVM(type.Channel)
def _(type, refine_to):
    return _llvmChannelType()
       
@codegen.convertTypeToC(type.Channel)
def _(type, refine_to):
    """A ``channel`` is mapped to a ``__hlt_channel *``. The type is defined in
    |hilti_intern.h|:
    
    .. literalinclude:: /libhilti/hilti_intern.h
       :start-after: %doc-hlt_channel-start
       :end-before:  %doc-hlt_channel-end
       
    """
    return _llvmChannelType()

@codegen.when(instructions.channel.New)
def _(self, i):
    t = self.llvmTypeConvertToC(i.op1().value())
    # TODO: Get the size of the operand.
    #tsize = llvm.core.Constant.sizeof(t)   # Size of the type in bytes.
    tsize = 4

    nodesize = instruction.ConstOperand(constant.Constant(tsize, type.Integer(32)))
    result = self.llvmGenerateCCallByName("__Hlt::channel_new", [nodesize], [type.Integer(32)])
    self.llvmStoreInTarget(i.target(), result)

@codegen.when(instructions.channel.Write)
def _(self, i):
    pass

@codegen.when(instructions.channel.Read)
def _(self, i):
    pass

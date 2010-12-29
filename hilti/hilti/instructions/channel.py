# $Id$
"""
.. hlt:type:: channel

   A channel is a high-performance data type for I/O operations that is
   designed to efficiently transfer large volumes of data both between the
   host application and HILTI and intra-HILTI components.

   The channel behavior depends on its type parameters which enable fine-tuning
   along the following dimensions:

       * Capacity. The capacity represents the maximum number of items a
         channel can hold. A full bounded channel cannot accomodate further
         items and a reader must first consume an item to render the channel
         writable again. By default, channels are unbounded and can grow
         arbitrarily large.
"""

import llvm.core

import hilti.block as block
import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("channel", 8, c="hlt::channel *")
class Channel(type.Container, type.Parameterizable):
    """Type for ``channel``.

    t:  ~~ValueType - Type of the channel items.
    """
    def __init__(self, t, location=None):
        super(Channel, self).__init__(t, location)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::channel_to_string";
        return typeinfo

@hlt.overload(New, op1=cType(cChannel), op2=cOptional(cIntegerOfWidth(64)), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a channel storing elements of type *op1*.
    *op2* defines the channel's capacity, i.e., the maximal number of items it
    can store. The capacity defaults to zero, which creates a channel of
    unbounded capacity.
    """
    def _codegen(self, cg):
        t = operand.Type(self.op1().value().itemType())
        cap = self.op2() if self.op2() else operand.Constant(constant.Constant(0, type.Integer(64)))
        result = cg.llvmCallC("hlt::channel_new", [t, cap])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("channel.write", op1=cReferenceOf(cChannel), op2=cItemTypeOfOp(1))
class Write(BlockingInstruction):
    """Writes an item into the channel referenced by *op1*. If the channel is
    full, the instruction blocks until a slot becomes available.
    """
    def cCall(self, cg):
        op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
        return ("hlt::channel_write_try", [self.op1(), op2])

@hlt.instruction("channel.write_try", op1=cReferenceOf(cChannel), op2=cItemTypeOfOp(1))
class WriteTry(Instruction):
    """Writes an item into the channel referenced by *op1*. If the channel is
    full, the instruction raises a ``WouldBlock`` exception.
    """
    def _codegen(self, cg):
        op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
        cg.llvmCallC("hlt::channel_write_try", [self.op1(), op2])

@hlt.instruction("channel.read", op1=cReferenceOf(cChannel), target=cItemTypeOfOp(1))
class Read(BlockingInstruction):
    """Returns the next channel item from the channel referenced by *op1*. If
    the channel is empty, the instruction blocks until an item becomes
    available.
    """
    def cCall(self, cg):
        return ("hlt::channel_read_try", [self.op1()])

    def cResult(self, cg, result):
        item_type = self.target().type()
        nodep = cg.builder().bitcast(result, llvm.core.Type.pointer(cg.llvmType(item_type)))
        cg.llvmStoreInTarget(self, cg.builder().load(nodep))

@hlt.instruction("channel.read_try", op1=cReferenceOf(cChannel), target=cItemTypeOfOp(1))
class ReadTry(Instruction):
    """Returns the next channel item from the channel referenced by *op1*. If
    the channel is empty, the instruction raises a ``WouldBlock`` exception.
    """
    def _codegen(self, cg):
        voidp = cg.llvmCallC("hlt::channel_read_try", [self.op1()])
        item_type = self.target().type()
        nodep = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(item_type)))
        cg.llvmStoreInTarget(self, cg.builder().load(nodep))

@hlt.instruction("channel.size", op1=cReferenceOf(cChannel), target=cIntegerOfWidth(64))
class Size(Instruction):
    """Returns the current number of items in the channel referenced by *op1*.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::channel_size", [self.op1()])
        cg.llvmStoreInTarget(self, result)

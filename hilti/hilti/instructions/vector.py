# $Id$
"""
.. hlt:type:: vector

   A ``vector`` maps integer indices to elements of a specific type
   ``T``. Vector elements can be read and written. Indices are
   zero-based. For a read operation, all indices smaller or equal
   the largest index written so far are valid. If an index is read
   that has not been yet written to, type T's default value is
   returned. Read operations are fast, as are insertions as long as
   the indices of inserted elements are not larger than the vector's
   last index. Inserting can be expensive however if an index beyond
   the vector's current end is given.

   Vectors are forward-iterable. An iterator always corresponds to a
   specific index and it is therefore safe to modify the vector even
   while iterating over it. After any change, the iterator will
   locate the element which is now located at the index it is
   pointing at. Once an iterator has reached the end-position
   however, it will never return an element anymore (but raise an
   ~~InvalidIterator exception), even if more are added to the
   vector.
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type(None, 101, doc="iterator<:hlt:type:`vector`\<T>>", c="hlt_vector_iter")
class IteratorVector(type.Iterator):
    """Type for iterating over ``vector``.

    t: ~~Vector - The Vector type.
    """
    def __init__(self, t, location=None):
        super(IteratorVector, self).__init__(t, location)

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        return cg.TypeInfo(self)

    def llvmType(self, cg):
        return llvm.core.Type.struct([cg.llvmTypeGenericPointer(), llvm.core.Type.int(64)])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """Vectors iterators are initially equivalent to ``vector.end``."""
        # Must match with what the C implementation expects as end() marker.
        return llvm.core.Constant.struct([llvm.core.Constant.null(cg.llvmTypeGenericPointer()), cg.llvmConstInt(0, 64)])

    ### Overridden from Iterator.

    def derefType(self):
        t = self.parentType().itemType()
        return t if t else type.Any()

@hlt.type("vector", 15, c="hlt_vector *")
class Vector(type.Container, type.Constructable, type.Iterable):
    """Type for a ``vector``.

    t: ~~Type - The element type of the vector.
    """
    def __init__(self, t, location=None):
        super(Vector, self).__init__(t, location)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::vector_to_string"
        return typeinfo

    ### Overriden from Iterable.

    def iterType(self):
        return IteratorVector(self, location=self.location())

    ### Overridden from Constructable.

    def validateCtor(self, vld, val):
        if not isinstance(val, list):
            vld.error(self, "not a Python list")

        for op in val:
            op.validate(vld)
            if not op.canCoerceTo(self.itemType()):
                vld.error(self, "vector element not coercable to %s" % self.itemType())

    def llvmCtor(self, cg, val):
        default = (self.itemType().llvmDefault(cg), self.itemType())
        vector = cg.llvmCallC("hlt::vector_new", [default], abort_on_except=True)
        vecop = operand.LLVM(vector, type.Reference(self))
        size = operand.Constant(constant.Constant(len(val), type.Integer(64)))
        cg.llvmCallC("hlt::vector_reserve", [vecop, size], abort_on_except=True)

        for o in val:
            o = o.coerceTo(cg, self.itemType())
            cg.llvmCallC("hlt::vector_push_back", [vecop, o], abort_on_except=True)

        return vector

    def outputCtor(self, printer, val):
        printer.printType(self)
        printer.output("(")
        first = True
        for op in val:
            if not first:
                printer.output(", ")
            op.output(printer)
            first = False
        printer.output(")")

@hlt.overload(New, op1=cType(cVector), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a vector of type *op1*."""
    def _codegen(self, cg):
        t = self.op1().value().itemType()
        default = operand.LLVM(cg.llvmConstDefaultValue(t), t)
        result = cg.llvmCallC("hlt::vector_new", [default])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("vector.get", op1=cReferenceOf(cVector), op2=cIntegerOfWidth(64), target=cItemTypeOfOp(1))
class Get(Instruction):
    """Returns the element at index *op2* in vector *op1*."""
    def _codegen(self, cg):
        t = self.op1().type().refType().itemType()
        op2 = self.op2().coerceTo(cg, type.Integer(64))
        voidp = cg.llvmCallC("hlt::vector_get", [self.op1(), op2])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.instruction("vector.set", op1=cReferenceOf(cVector), op2=cIntegerOfWidth(64), op3=cItemTypeOfOp(1))
class Set(Instruction):
    """Sets the element at index *op2* in vector *op1* to *op3."""
    def _codegen(self, cg):
        op2 = self.op2().coerceTo(cg, type.Integer(64))
        op3 = self.op3().coerceTo(cg, self.op1().type().refType().itemType())
        cg.llvmCallC("hlt::vector_set", [self.op1(), op2, op3])

@hlt.instruction("vector.push_back", op1=cReferenceOf(cVector), op2=cItemTypeOfOp(1))
class Set(Instruction):
    """Sets the element at index *op2* in vector *op1* to *op3."""
    def _codegen(self, cg):
        op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
        cg.llvmCallC("hlt::vector_push_back", [self.op1(), op2])

@hlt.instruction("vector.size", op1=cReferenceOf(cVector), target=cIntegerOfWidth(64))
class Size(Instruction):
    """Returns the current size of the vector *op1*, which is the largest
    accessible index plus one."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::vector_size", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("vector.reserve", op1=cReferenceOf(cVector), op2=cIntegerOfWidth(64))
class Reserve(Instruction):
    """Reserves space for at least *op2* elements in vector *op1*. This
    operations does not change the vector in any observable way but rather
    gives a hint to the implementation about the size that will be needed. The
    implemenation may use this information to avoid unnecessary memory
    allocations.
    """
    def _codegen(self, cg):
        op2 = self.op2().coerceTo(cg, type.Integer(64))
        cg.llvmCallC("hlt::vector_reserve", [self.op1(), op2])

@hlt.overload(Begin, op1=cReferenceOf(cVector), target=cIteratorVector)
class Begin(Operator):
    """Returns an iterator representing the first element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::vector_begin", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(End, op1=cReferenceOf(cVector), target=cIteratorVector)
class End(Operator):
    """Returns an iterator representing the position one after the last element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::vector_end", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Incr, op1=cIteratorVector, target=cIteratorVector)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::vector_iter_incr", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Deref, op1=cIteratorVector, target=cDerefTypeOfOp(1))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at.
    """
    def _codegen(self, cg):
        t = self.target().type()
        voidp = cg.llvmCallC("hlt::vector_iter_deref", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.overload(Equal, op1=cIteratorVector, op2=cIteratorVector, target=cBool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::vector_iter_eq", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

# $Id$
"""
Lists
~~~~~

A ``list`` stores elements of a specific tyep T in a double-linked list,
allowing for fast insert operations at the expense of expensive random access.

Lists are forward-iterable. Note that it is generally safe to modify a list
while iterating over it. The only problematic case occurs if the very element
get removed from the list to which an iterator is pointing. If so, accessing
the iterator will raise an ~~InvalidIterator exception. 
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type(None, 102)
class IteratorList(type.Iterator):
    """Type for iterating over ``List``.
    
    t: ~~List - The list type.
    """
    def __init__(self, t, location=None):
        super(IteratorList, self).__init__(t, location)
        
    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_list_iter"
        return typeinfo
    
    def llvmType(self, cg):
        """An ``iterator<list<T>>`` is mapped to ``struct hlt_list_iter``."""
        return llvm.core.Type.struct([cg.llvmTypeGenericPointer()] * 2)

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """List iterators are initially equivalent to ``list.end``."""
        # Must match with what the C implementation expects as end() marker.
        return llvm.core.Constant.struct([llvm.core.Constant.null(cg.llvmTypeGenericPointer())] * 2)

    ### Overridden from Iterator.
    
    def derefType(self):
        t = self.parentType().itemType() 
        return t if t else type.Any()
    
@hlt.type("list", 16)
class List(type.Container, type.Constructable, type.Iterable):
    """Type for a ``list``.
    
    t: ~~Type - The element type of the list, None for ``list<*>``.
    """
    def __init__(self, t, location=None):
        super(List, self).__init__(t, location)
        
    ### Overridden from HiltiType.
    
    def llvmType(self, cg):
        """A ``list`` is mapped to a ``hlt_list *``."""
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_list *"
        typeinfo.to_string = "hlt::list_to_string"
        return typeinfo

    ### Overridden from Constructable.

    def validateCtor(self, vld, val):
        if not isinstance(val, list):
            vld.error(self, "not a Python list")
            
        for op in val:
            op.validate(vld)
            if not op.canCoerceTo(self.itemType()):
                vld.error(self, "list element not coercable to %s" % self.itemType())
    
    def llvmCtor(self, cg, val):
        list = cg.llvmCallC("hlt::list_new", [operand.Type(self.itemType())], abort_on_except=True)
        listop = operand.LLVM(list, type.Reference(self))
        
        for o in val:
            o = o.coerceTo(cg, self.itemType())
            cg.llvmCallC("hlt::list_push_back", [listop, o], abort_on_except=True)
            
        return list

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
        
    ### Overriden from Iterable.
        
    def iterType(self):
        return IteratorList(self, location=self.location())

@hlt.overload(New, op1=cType(cList), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a list of type *op1*. 
    """
    def _codegen(self, cg):
        t = operand.Type(self.op1().value().itemType())
        result = cg.llvmCallC("hlt::list_new", [t])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("list.push_back", op1=cReferenceOf(cList), op2=cItemTypeOfOp(1))
class PushBack(Instruction):
    """Appends *op2* to the list in reference by *op1*."""
    def _codegen(self, cg):
        op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
        cg.llvmCallC("hlt::list_push_back", [self.op1(), op2])

@hlt.instruction("list.push_front", op1=cReferenceOf(cList), op2=cItemTypeOfOp(1))
class PushFront(Instruction):
    """Prepends *op2* to the list in reference by *op1*."""
    def _codegen(self, cg):
        op2 = self.op2().coerceTo(cg, self.op1().type().refType().itemType())
        cg.llvmCallC("hlt::list_push_front", [self.op1(), op2])

@hlt.instruction("list.pop_front", op1=cReferenceOf(cList), target=cItemTypeOfOp(1))
class PopFront(Instruction): 
    """Removes the first element from the list referenced by *op1* and returns
    it. If the list is empty, raises an ~~Underflow exception."""
    def _codegen(self, cg):
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_pop_front", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))
    
@hlt.instruction("list.pop_back", op1=cReferenceOf(cList), target=cItemTypeOfOp(1))
class PopBack(Instruction):
    """Removes the last element from the list referenced by *op1* and returns
    it. If the list is empty, raises an ~~Underflow exception."""
    def _codegen(self, cg):
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_pop_back", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.instruction("list.front", op1=cReferenceOf(cList), target=cItemTypeOfOp(1))
class Front(Instruction):
    """Returns the first element from the list referenced by *op1*. If the
    list is empty, raises an ~~Underflow exception."""
    def _codegen(self, cg):
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_front", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.instruction("list.back", op1=cReferenceOf(cList), target=cItemTypeOfOp(1))
class Back(Instruction):
    """Returns the last element from the list referenced by *op1*. If the
    list is empty, raises an ~~Underflow exception."""
    def _codegen(self, cg):
        t = self.op1().type().refType().itemType()
        voidp = cg.llvmCallC("hlt::list_back", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))
 
@hlt.instruction("list.size", op1=cReferenceOf(cList), target=cIntegerOfWidth(64))
class Size(Instruction):
    """Returns the current size of the list reference by *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::list_size", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("list.erase", op1=cIteratorList)
class Erase(Instruction):
    """Removes the list element located by the iterator in *op1*."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::list_erase", [self.op1()])

@hlt.instruction("list.insert", op1=cDerefTypeOfOp(2),  op2=cIteratorList)
class Insert(Instruction):
    """Inserts the element *op1* into a list before the element located by the
    iterator in *op2*. If the iterator is at the end position, the element
    will become the new list tail."""
    def _codegen(self, cg):
        op1 = self.op1().coerceTo(cg, self.op2().type().derefType())
        cg.llvmCallC("hlt::list_insert", [op1, self.op2()])

@hlt.overload(Begin, op1=cReferenceOf(cList), target=cIteratorList)
class Begin(Instruction):
    """Returns an iterator representing the first element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::list_begin", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(End, op1=cReferenceOf(cList), target=cIteratorList)
class End(Instruction):
    """Returns an iterator representing the position one after the last element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::list_end", [self.op1()])
        cg.llvmStoreInTarget(self, result)
    
@hlt.overload(Incr, op1=cIteratorList, target=cIteratorList)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::list_iter_incr", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Deref, op1=cIteratorList, target=cDerefTypeOfOp(1))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at. 
    """
    def _codegen(self, cg):
        t = self.target().type()
        voidp = cg.llvmCallC("hlt::list_iter_deref", [self.op1()])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(t)))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))

@hlt.overload(Equal, op1=cIteratorList, op2=cIteratorList, target=cBool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::list_iter_eq", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)


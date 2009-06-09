# $Id$
"""
Lists
~~~~~
"""

_doc_type_description = """A ``list`` stores elements of a specific
tyep T in a double-linked list, allowing for fast insert
operations at the expense of expensive random access. 

Lists are forward-iterable. Note that it is generally safe to modify
a list while iterating over it. The only problematic case occurs if
the very element is removed from the list to which an iterator is
pointing. If so, accessing the iterator will raise an
~~InvalidIterator exception. 
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(New, op1=isType(list), target=referenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a list storing elements of type
    *op1*.
    """
    pass


@instruction("list.push_back", op1=referenceOf(list), op2=itemTypeOfOp(1))
class PushBack(Instruction):
    """Appends *op2* to the list in reference by *op1*."""

@instruction("list.push_front", op1=referenceOf(list), op2=itemTypeOfOp(1))
class PushFront(Instruction):
    """Prepends *op2* to the list in reference by *op1*."""

@instruction("list.pop_front", op1=referenceOf(list), target=itemTypeOfOp(1))
class PopFront(Instruction): 
    """Removes the first element from the list referenced by *op1* and returns
    it. If the list is empty, raises an ~~Underflow exception."""
    
@instruction("list.pop_back", op1=referenceOf(list), target=itemTypeOfOp(1))
class PopBack(Instruction):
    """Removes the last element from the list referenced by *op1* and returns
    it. If the list is empty, raises an ~~Underflow exception."""

@instruction("list.front", op1=referenceOf(list), target=itemTypeOfOp(1))
class Front(Instruction):
    """Returns the first element from the list referenced by *op1*. If the
    list is empty, raises an ~~Underflow exception."""

@instruction("list.back", op1=referenceOf(list), target=itemTypeOfOp(1))
class Back(Instruction):
    """Returns the last element from the list referenced by *op1*. If the
    list is empty, raises an ~~Underflow exception."""
 
@instruction("list.size", op1=referenceOf(list), target=integerOfWidth(64))
class Size(Instruction):
    """Returns the current size of the list reference by *op1*."""

@instruction("list.erase", op1=iteratorList)
class Erase(Instruction):
    """Removes the list element located by the iterator in *op1*."""

@instruction("list.insert", op1=derefTypeOfOp(2),  op2=iteratorList)
class Insert(Instruction):
    """Inserts the element *op1* into a list before the element located by the
    iterator in *op2*. If the iterator is at the end position, the element
    will become the new list tail."""

@instruction("list.begin", op1=referenceOf(list), target=iteratorList)
class Begin(Instruction):
    """Returns an iterator representing the first element of *op1*."""
    pass

@instruction("list.end", op1=referenceOf(list), target=iteratorList)
class End(Instruction):
    """Returns an iterator representing the position one after the last element of *op1*."""
    pass    
    
@overload(Incr, op1=iteratorList, target=iteratorList)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    pass

@overload(Deref, op1=iteratorList, target=derefTypeOfOp(1))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at. 
    """
    pass

@overload(Equal, op1=iteratorList, op2=iteratorList, target=bool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    pass

# $Id$
"""
Vectors
~~~~~~~
"""

_doc_type_description = """A vector maps integer indices to elements
of a specific type T. Vector elements can be read and written.
Indices are zero-based. For a read operation, all indices smaller or
equal the largest index written so far are valid. If an index is
read that has not been yet written to, type T's default value is
returned. Read operations are fast, as are insertions as long as the
indices of inserted elements are not larger than the vector's last
index. Inserting can be expensive however if an index beyond the
vector's current end is given. 

Vectors are forward-iterable. An iterator always corresponds to a specific
index and it is therefore safe to modify the vector even while iterating over
it. After any change, the iterator will locate the element which is now
located at the index it is pointing at. Once an iterator has reached the
end-position however, it will never return an element anymore (but raise an
~~InvalidIterator exception), even if more are added to the vector. 
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(New, op1=isType(vector), target=referenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a vector storing elements of the same type
    as the target is referencing. 
    """
    pass

@instruction("vector.get", op1=referenceOf(vector), op2=integerOfWidth(64), target=itemTypeOfOp(1))
class Get(Instruction):
    """Returns the element at index *op2* in vector *op1*."""
    
@instruction("vector.set", op1=referenceOf(vector), op2=integerOfWidth(64), op3=itemTypeOfOp(1))
class Set(Instruction):
    """Sets the element at index *op2* in vector *op1* to *op3."""

@instruction("vector.size", op1=referenceOf(vector), target=integerOfWidth(64))
class Size(Instruction):
    """Returns the current size of the vector *op1*, which is the largest
    accessible index plus one."""

@instruction("vector.reserve", op1=referenceOf(vector), op2=integerOfWidth(64))
class Reserve(Instruction):
    """Reserves space for at least *op2* elements in vector *op1*. This
    operations does not change the vector in any observable way but rather
    gives a hint to the implementation about the size that will be needed. The
    implemenation may use this information to avoid unnecessary memory
    allocations.
    """ 

@instruction("vector.begin", op1=referenceOf(vector), target=iteratorVector)
class Begin(Instruction):
    """Returns an iterator representing the first element of *op1*."""
    pass

@instruction("vector.end", op1=referenceOf(vector), target=iteratorVector)
class End(Instruction):
    """Returns an iterator representing the position one after the last element of *op1*."""
    pass    
    
@overload(Incr, op1=iteratorVector, target=iteratorVector)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    pass

@overload(Deref, op1=iteratorVector, target=derefTypeOfOp(1))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at. 
    """
    pass

@overload(Equal, op1=iteratorVector, op2=iteratorVector, target=bool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    pass

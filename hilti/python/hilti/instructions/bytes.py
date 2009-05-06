# $Id$
"""
Bytes
~~~~~
"""

_doc_type_description = """``Bytes`` is a data type for storing sequences of raw
bytes. It is optimized for storing and operating on large amounts of
unstructured data. In particular, it provides efficient subsequence and
append operations. Bytes are forward-iterable. 

There is a constructor for ``bytes`` resembling string syntax: ``b"abcdef"``
creates a new ``bytes`` object and initialized it with six bytes. Note that
such ``bytes`` instances are *not* constants but can be modified later on.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(New, op1=isType(bytes), target=referenceOf(bytes))
class New(Operator):
    """
    Allocates a new *bytes* instancem, which will be initially empty.
    """
    pass

@overload(Incr, op1=iteratorBytes, target=iteratorBytes)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    pass

@overload(Deref, op1=iteratorBytes, target=integerOfWidth(8))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at. 
    """
    pass

@overload(Equal, op1=iteratorBytes, op2=iteratorBytes, target=bool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    pass

@instruction("bytes.length", op1=referenceOf(bytes), target=integerOfWidth(32))
class Length(Instruction):
    """Returns the number of bytes stored in *op1*."""
    pass

@instruction("bytes.empty", op1=referenceOf(bytes), target=bool)
class Empty(Instruction):
    """Returns true if *op1* is empty. Note that using this instruction is
    more efficient than comparing the result of ``bytes.length`` to zero."""
    pass

@instruction("bytes.append", op1=referenceOf(bytes), op2=referenceOf(bytes))
class Append(Instruction):
    """Appends *op2* to *op1*."""
    pass

@instruction("bytes.sub", op1=iteratorBytes, op2=iteratorBytes, target=referenceOf(bytes))
class Sub(Instruction):
    """Extracts the subsequence between *op1* and *op2* from an existing
    *bytes* instance and returns it in a new ``bytes`` instance. The element
    at *op2* is not included."""
    pass

@instruction("bytes.offset", op1=referenceOf(bytes), op2=integerOfWidth(32), target=iteratorBytes)
class Offset(Instruction):
    """Returns an iterator representing the offset *op2* in *op1*."""
    pass

@instruction("bytes.begin", op1=referenceOf(bytes), target=iteratorBytes)
class Begin(Instruction):
    """Returns an iterator representing the first element of *op1*."""
    pass

@instruction("bytes.end", op1=referenceOf(bytes), target=iteratorBytes)
class End(Instruction):
    """Returns an iterator representing the position one after the last element of *op1*."""
    pass

@instruction("bytes.diff", op1=iteratorBytes, op2=iteratorBytes, target=integerOfWidth(32))
class Diff(Instruction):
    """Returns the number of bytes between *op1* and *op2*."""
    pass

#@instruction("bytes.find", op1=Reference, op2=Reference, target=Integer)
#class Find(Instruction):
#    """Searches *op2* in *op1*, returning the (zero-based) start index if it
#    finds it. Returns -1 if it does not find *op2* in *op1*."""
#    pass
#
#@instruction("bytes.cmp", op1=Reference, op2=Reference, target=Bool)
#class Cmp(Instruction):
#    """Compares *op1* with *op2* and returns *True* if their contents match.
#    Returns *False* otherwise."""
#    pass

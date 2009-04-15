"""
Operators
~~~~~~~~~

An *operator* is a instruction that it not tied to a particular type but 
is overloaded for arguments of different types to implement
semantically similar operations. Note that the specifics of each operation is
defined on a per-type basis, and documented there. For example, the *incr* operator (which
increments its argument by one) will add 1 to an integer yet move to the
next element for a container iterator.

Todo: We don't do (almost) any type checking for the operands as we don't have
the infrastructure to check them on a type-specific basis. Need to add that.
"""

from hilti.core.type import *
from hilti.core.instruction import *

@operator("new", op1=HeapType, target=Reference)
class New(Operator):
    """
    Allocates a new instance of type *op1*.
    """
    pass

@operator("incr", op1=HiltiType, target=HiltiType)
class Incr(Operator):
    """
    Increments *op1* by one and returns the result. *op1* is not modified.
    """
    pass

@operator("deref", op1=HiltiType, target=HiltiType)
class Deref(Operator):
    """
    Dereferences *op1* and returns the result.
    """
    pass

@operator("equal", op1=HiltiType, op2=HiltiType, target=Bool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*
    """
    pass



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

@operator("unpack", op1=IteratorBytes, op2=IteratorBytes, op3=Any, target=Tuple)
class Unpack(Operator):
    """Unpacks an instance of a particular type (as determined by *target*;
    see below) from the the binary data identified by the range from *op1* to
    *op2*. *op3* may provide type-specific hints about the layout of the raw
    bytes. The operator returns a ``tuple<T, iterator<bytes>>``, in the first
    component is the newly unpacked instance and the second component is
    locates the first bytes that has *not* been consumed anymore. 
    
    Raises ~~UnpackError if the raw bytes are not as expected (and
    that fact can be verified); this includes the case that the
    provided range does not contain sufficient bytes for unpacking
    one instance.
    
    Note: All implementations of this operator must be able to deal with bytes
    located at arbitrary, not necessarily aligned positions.
    """
    pass



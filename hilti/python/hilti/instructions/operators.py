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

from hilti.core.instruction import *
from hilti.core.constraints import *

@operator("new", op1=isType(heapType), op2=optional(any), target=referenceOfOp(1))
class New(Operator):
    """
    Allocates a new instance of type *op1*.
    """
    pass

@operator("incr", op1=hiltiType, target=sameTypeAsOp(1))
class Incr(Operator):
    """
    Increments *op1* by one and returns the result. *op1* is not modified.
    """
    pass

@operator("deref", op1=hiltiType, target=hiltiType)
class Deref(Operator):
    """
    Dereferences *op1* and returns the result.
    """
    pass

@operator("equal", op1=hiltiType, op2=hiltiType, target=bool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*
    """
    pass

# FIXME: For op2, what we really want is "is of type enum Hilti::Packed". But
# how can we express that?
def unpackTarget(constraint):
    """A constraint function that ensures that the operand is of the
    tuple type expected for the ~~Unpack operator's target. *t* is an
    additional constraint function for the tuple's first element.
    """ 
    return isTuple([constraint, iteratorBytes])

# FIXME: How can we check op3 in a reasonably generic way?
@operator("unpack", op1=isTuple([iteratorBytes,iteratorBytes]), op2=enum, op3=optional(any), target=unpackTarget(any))
class Unpack(Operator):
    """Unpacks an instance of a particular type (as determined by *target*;
    see below) from the binary data enclosed by the iterator tuple *op1*. *op2*
    defines the binary layout as an enum of type ``Hilti::Packed``. Depending
    on *op2*, *op3* is may be an additional, format-specific parameter with
    further information about the binary layout. The operator returns a
    ``tuple<T, iterator<bytes>>``, in the first component is the newly
    unpacked instance and the second component is locates the first bytes that
    has *not* been consumed anymore. 
    
    Raises ~~UnpackError if the raw bytes are not as expected (and
    that fact can be verified); this includes the case that the
    provided range does not contain sufficient bytes for unpacking
    one instance.

    Note: The ``unpack`` operator uses a generic implementation able to handle all data
    types. Different from most other operators, it's implementation is not
    overloaded on a per-type based. However, each type must come with an
    ~~unpack decorated function, which the generic operator implementatin
    relies on for doing the unpacking.
    """
    pass

@overload(Unpack, op1=isTuple([iteratorBytes,iteratorBytes]), op2=enum, op3=optional(any), target=unpackTarget(any))
class GenericUnpack(Operator):
    """The generic implementation of the ``unpack operator``. This operator is
    not overloaded on a per-type basis."""
    pass
    
@operator("assign", op1=valueType, target=sameTypeAsOp(1))
class Assign(Operator):
    """Assigns *op1* to the target.
    
    There is a short-cut syntax: instead of using the standard form ``t = assign op``, one
    can just write ``t = op``.
    
    Note: The ``assign`` operator uses a generic implementation able to handle all data
    types. Different from most other operators, it's implementation is not
    overloaded on a per-type based. 
    """
    
@overload(Assign, op1=valueType, target=sameTypeAsOp(1))
class GenericAssign(Operator):
    """The generic implementation of the ``assign operator``. This operator is
    not overloaded on a per-type basis."""
    pass



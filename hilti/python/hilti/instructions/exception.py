# $Id$
"""
Exceptions
~~~~~~~~~~
"""

_doc_type_description = """The *exception* data type represents an exception
that can divert control up the calling stack to the closest function prepared
to handle it.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

# We check the second argument's type in checker.exception.
@overload(New, op1=isType(exception), op2=optional(any), target=referenceOfOp(1))
class New(Operator):
    """Allocates a new exception instance. If the exception type given in
    *op1* requires an argument, that must be passed in *op2*.
    """
    pass

@instruction("exception.throw", op1=referenceOf(exception), terminator=True)
class Throw(Instruction):
    """
    Throws an exception, diverting control-flow up the stack to the closest
    handler.
    """

    



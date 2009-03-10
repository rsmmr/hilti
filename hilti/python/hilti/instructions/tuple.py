# $Id$
#
# Instruction for the integer data type.
""" The *tuple* data type represents ordered tuples of other values, which can
be of mixed type. Tuple are however statically types and therefore need the
individual types to be specified as type parameters, e.g.,
``tuple<int32,string,bool>`` to represent a 3-tuple of ``int32``, ``string``,
and ``bool``. Tuple values are enclosed in parentheses with the individual
components separated by commas, e.g., ``(42, "foo", True)``. 
"""

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("tuple.assign", op1=Tuple, target=Tuple)
class Assign(Instruction):
    """Assigns *op1* to the target."""

@instruction("tuple.index", op1=Tuple, op2=Integer(32), target=Any)
class Index(Instruction):
    """ Returns the tuple's value with index *op2*. The index is zero-based.
    *op2* must be an integer *constant*.
    """
    
        

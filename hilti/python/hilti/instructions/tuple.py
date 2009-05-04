# $Id$
"""
Tuples
~~~~~~
"""

_doc_type_description = """The *tuple* data type represents ordered tuples of
other values, which can be of mixed type. Tuple are however statically types
and therefore need the individual types to be specified as type parameters,
e.g., ``tuple<int32,string,bool>`` to represent a 3-tuple of ``int32``,
``string``, and ``bool``. Tuple values are enclosed in parentheses with the
individual components separated by commas, e.g., ``(42, "foo", True)``.  If
not explictly initialized, tuples are set to their components' default values
initially.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *

@constraint("any")
@refineType(refineIntegerConstant(32))
def _isElementIndex(ty, op, i):
    if not op or not isinstance(op, ConstOperand):
        return (False, "must be a constant")

    if not isinstance(ty, type.Integer) or ty.width() != 32:
        return (False, "must be of type int<32> but is %s" % ty)
    
    idx = op.value()
    if idx >= 0 and idx < len(i.op1().type().types()):
        return (True, "")
    
    return (False, "is out of range")

@constraint("any")
def _hasElementType(ty, op, i):
    t = i.op1().type().types()[i.op2().value()]
    
    if t == ty:
        return (True, "")
    
    return (False, "type must be %s but is %s" % (t, ty))

@instruction("tuple.assign", op1=tuple, target=sameTypeAsOp(1))
class Assign(Instruction):
    """Assigns *op1* to the target."""

@instruction("tuple.index", op1=tuple, op2=_isElementIndex, target=_hasElementType)
class Index(Instruction):
    """ Returns the tuple's value with index *op2*. The index is zero-based.
    *op2* must be an integer *constant*.
    """
    
        

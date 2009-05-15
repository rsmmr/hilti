# $Id$
"""
Overlay
~~~~~~~
"""

_doc_type_description = """The *overlay* data type is TODO ...."""

from hilti.core.instruction import *
from hilti.core.constraints import *

@constraint("string")
def _fieldName(ty, op, i):
    if not op or not isinstance(op, ConstOperand):
        return (False, "field name must be constant")
    
    if not isinstance(ty, type.String):
        return (False, "field name must be a string")

    return (i.op1().type().field(op.value()) != None, 
            "%s is not a field name in %s" % (op.value(), i.op1().type()))
    
@constraint("any")
def _fieldType(ty, op, i):
    ft = i.op1().type().field(i.op2().value()).type
    return (ty == ft, "target type must be %s" % ft)

@instruction("overlay.attach", op1=overlay, op2=iteratorBytes)
class Attach(Instruction):
    """Associates the overlay in *op1* with a bytes object, aligning the
    overlays offset 0 with the position specified by *op2*. The *attach*
    operation must be performed before any other ``overlay`` instruction is
    used on *op1*.
    """

@instruction("overlay.get", op1=overlay, op2=_fieldName, target=_fieldType)
class Get(Instruction):
    """Unpacks the field named *op2* from the bytes object attached to the
    overlay *op1*. The field name must be a constant, and the type of the
    target must match the field's type. 
    
    The instruction throws an OverlayNotAttached exception if
    ``overlay.attach`` has not been executed for *op1* yet.
    """

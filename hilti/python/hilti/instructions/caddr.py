# $Id$
"""
CAddr
~~~~~
"""

_doc_type_description = """The *caddr* data type stores the physical memory
address of a HILTI object; it is primarily a tool for passing such an address
on to an external C program. Note that there's no type information associated
with a *caddr* value and thus care has to be taken when using it to access
memory. 
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

_pair = isTuple([integerOfWidth(32), iteratorBytes])

@constraint("function")
def _exportedFunction(ty, op, i):
    if not isinstance(ty, type.Function):
        return (False, "must be a function name")
    
    if not isinstance(op, IDOperand):
        return (False, "must be a function name")

    # TODO: Should check that we reference a constant here. What how?
    
    return (True, "")

@instruction("caddr.function", op1=_exportedFunction, target=isTuple([caddr, caddr]))
class Function(Instruction):
    """Returns the physical address of a function. The function must be of
    linkage ~~EXPORT, and the instruction returns two separate addresses for
    ~~HILTI: the first is that of the compiled function itself, and the second
    is that of the corresponding ~~resume function. For functions of linkage
    ~~C and ~~HILTI, only the first element of the target tuple is used, and
    the second is undefined. *op1* must be a constant. 
    """
    

# $Id$
#
# String data type.

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("string.add", op1=String, op2=String, target=String)
class Add(Instruction):
    """Concatenates operand 2 to operand 1."""
    pass

        

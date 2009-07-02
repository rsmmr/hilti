# $Id$
"""
Booleans
~~~~~~~~
"""

_doc_type_description = """The *bool* data type represents boolean values.
The two boolean constants are ``True`` and ``False``. If not explictly
initialized, booleans are set to ``False`` initially.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@instruction("bool.and", op1=bool, op2=bool, target=bool)
class And(Instruction):
    """
    Computes the logical 'and' of *op1* and *op2*.
    """
    
@instruction("bool.or", op1=bool, op2=bool, target=bool)
class Or(Instruction):
    """
    Computes the logical 'or' of *op1* and *op2*.
    """
    
@instruction("bool.not", op1=bool, target=bool)
class Not(Instruction):
    """
    Computes the logical 'not' of *op1*.
    """
    
    



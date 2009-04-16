# $Id$
"""
Enums
~~~~~
"""

_doc_type_description = """The *enums* data type represents a selection of
unique values, identified by labels. Enums must be defined in global space:

   enum Foo { Red, Green, Blue }

The individual labels can then be used as global identifier. In addition to
the user-defined labels, each *enum* implicitly defines one additional label
called ''Undef''. If not explictly initialized, enums are set to ``Undef``
initially.
"""

from hilti.core.type import *
from hilti.core.instruction import *
from hilti.instructions.operators import *

@overload(Equal, op1=Enum, op2=Enum, target=Bool)
class Equals(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    pass

@instruction("enum.assign", op1=Enum, target=Enum)
class Assign(Instruction):
    """
    Assign *op1* to the target. 
    """





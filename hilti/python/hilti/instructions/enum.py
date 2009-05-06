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

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(Equal, op1=enum, op2=sameTypeAsOp(1), target=bool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    pass





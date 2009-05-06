# $Id$
"""
References
~~~~~~~~~~
"""

_doc_type_description = """The ``ref<T>`` data type encapsulates references
to dynamically allocated, garbage-collected objects of type *T*. The special
reference constant ``Null`` can used as place-holder for invalid references.  

If not explictly initialized, references are set to ``Null`` initially.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *

@instruction("ref.cast.bool", op1=reference, target=bool)
class CastBool(Instruction):
    """
    Converts *op1* into a boolean. The boolean's value will be True if *op1*
    references a valid object, and False otherwise. 
    """

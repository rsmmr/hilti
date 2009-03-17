# $Id$
#
# Instructions for the ref data type.
#
""" 
The ``ref<T>`` data type encapsulates references to dynamically allocated,
garbage-collected objects of type *T*. The special reference constant ``Null``
can used as place-holder for invalid references.  
"""

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("ref.cast.bool", op1=Reference, target=Bool)
class CastBool(Instruction):
    """
    Converts *op1* into a boolean. The boolean's value will be True if *op1*
    references a valid object, and False otherwise. 
    """

@instruction("ref.assign", op1=Reference, target=Reference)
class Assign(Instruction):
    """Assigns *op1* to the target."""


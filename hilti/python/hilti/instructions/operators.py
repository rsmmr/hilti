"""
Operators
~~~~~~~~~

An *operator* is a instruction that it not tied to a particular type but 
operates on arguments of different types in a semantically similar way.
"""

from hilti.core.type import *
from hilti.core.instruction import *

@operator("new", op1=HeapType, target=Reference)
class New(Operator):
    """
    Allocates a new instance of type *op1*.
    """
    pass


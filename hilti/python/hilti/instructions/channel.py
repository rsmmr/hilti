# $Id$
#
# Instructions for the channel data type.
""" The ``channel`` data type... TODO.
"""

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("channel.new", op1=ValueType, target=Reference)
class New(Instruction):
    """Instantiates a new channel object of the type *op1* and returns a
    reference to it.
    """

@instruction("channel.write", op1=Reference, op2=ValueType)
class Write(Instruction):
    """Writes an element into the channel referenced by *op1*. If the channel is
    full, the caller blocks.
    """

@instruction("channel.read", op1=Reference, target=ValueType)
class Read(Instruction):
    """Returns the next channel element from the channel referenced by *op1*. If
    the channel is empty, the caller blocks.
    """

# $Id$
"""
Bitsets
~~~~~~~
"""

_doc_type_description = """The *bitset* data type groups a set of bits
together. In an instance, each of the bits is either set or not set. The
individual bits are identified by labels, which are declared in the bitsets
type declaration::

   bitset MyBits { Bit1, Bit2, Bit3 }

The individual labels can then be used with the ``bitset.*``
commands::

   local mybits: MyBits
   mybits = bitset.set mybits MyBits::Bit2
   
Bitset constants are created via the labels, and multiple bits can be set by
chaining them via the ``|`` operator::

   mybits = MyBits::Bit1 | MyBits::Bit2

Note: For efficiency reasons, HILTI supports only up to 64 bits per type. 

Todo: We don't support multi-value bitset constants yet. Need to add logic for ``BIT1 |
BIT2 | BIT3`` to parser.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(Equal, op1=bitset, op2=sameTypeAsOp(1), target=bool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    pass

@instruction("bitset.set", op1=bitset, op2=sameTypeAsOp(1), target=sameTypeAsOp(1))
class Set(Instruction):
    """
    Adds the bits set in *op2* to those set in *op1* and returns the result.
    """

@instruction("bitset.clear", op1=bitset, op2=sameTypeAsOp(1), target=sameTypeAsOp(1))
class Clear(Instruction):
    """
    Removes the bits set in *op2* from those set in *op1* and returns the result.
    """
    
@instruction("bitset.has", op1=bitset, op2=sameTypeAsOp(1), target=bool)
class Has(Instruction):
    """
    Returns true if all bits in *op2* are set in *op1*.
    """




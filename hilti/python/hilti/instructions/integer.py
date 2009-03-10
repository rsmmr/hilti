# $Id$
#
# Instruction for the integer data type.
"""
The *integer* data type represents signed integers of a fixed width. The width
is specified as part of the type name as, e.g., in ``int<16>`` for a 16-bit
integer. There are predefined shortcuts ``int8``, ``int16``, ``int32`` and
``int64``.
"""

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("int.add", op1=Integer, op2=Integer, target=Integer)
class Add(Instruction):
    """
    Calculates the sum of the two operands. Operands and target must be of
    same width. The result is calculated modulo 2^{width}. 
    """

@instruction("int.sub", op1=Integer, op2=Integer, target=Integer)
class Sub(Instruction):
    """
    Subtracts *op1* one from *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """
    
@instruction("int.mul", op1=Integer, op2=Integer, target=Integer)
class Mul(Instruction):
    """
    Multiplies *op1* with *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """

@instruction("int.div", op1=Integer, op2=Integer, target=Integer)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result. Operands and target must be
    of same width.  If the product overflows the range of the integer type,
    the result in undefined. 
    
    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    
@instruction("int.eq", op1=Integer, op2=Integer, target=Bool)
class Eq(Instruction):
    """
    Returns true iff *op1* equals *op2*. 
    """
    
@instruction("int.lt", op1=Integer, op2=Integer, target=Bool)
class Lt(Instruction):
    """
    Returns true iff *op1* is less than *op2*.
    """

@instruction("int.ext", op1=Integer, target=Integer)
class Ext(Instruction):
    """
    Zero-extends *op1* into an integer of the same width as the *target*. The
    width of *op1* must be smaller or equal that of the *target*. 
    """

@instruction("int.trunc", op1=Integer, target=Integer)
class Trunc(Instruction):
    """
    Bit-truncates *op1* into an integer of the same width as the *target*. The
    width of *op1* must be larger or equal that of the *target*. 
    """

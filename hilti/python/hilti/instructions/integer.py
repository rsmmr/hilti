# $Id$
#
# Instruction for the integer data type.
"""
The +integer+ data type represents signed integers of a fixed width. The width
is specified as part of the type name as, e.g., in +int:16+ for a 16-bit
integer. 
"""

from hilti.core.type import *
from hilti.core.instruction import *

def _unifyConstants(i):
    # We assign widths to integer constants by taking the largest widths of
    # all other integer operands/target.
    maxwidth = max([op.type().width() for op in (i.op1(), i.op2(), i.op3(), i.target()) if op and isinstance(op.type(), type.IntegerType)])
    
    if i.op1() and isinstance(i.op1(), ConstOperand) and isinstance(i.op1().type(), type.IntegerType):
        i.op1().setType(type.IntegerType(maxwidth))
        
    if i.op2() and isinstance(i.op2(), ConstOperand) and isinstance(i.op2().type(), type.IntegerType):
        i.op2().setType(type.IntegerType(maxwidth))

    if i.op3() and isinstance(i.op3(), ConstOperand) and isinstance(i.op3().type(), type.IntegerType):
        i.op3().setType(type.IntegerType(maxwidth))
        

@instruction("int.add", op1=Integer, op2=Integer, target=Integer, callback=_unifyConstants)
class Add(Instruction):
    """
    Calculates the sum of the two operands. Operands and target must be of
    same width. The result is calculated modulo 2^{width}. 
    """

@instruction("int.sub", op1=Integer, op2=Integer, target=Integer, callback=_unifyConstants)
class Sub(Instruction):
    """
    Subtracts *op1* one from *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """
    
@instruction("int.mul", op1=Integer, op2=Integer, target=Integer, callback=_unifyConstants)
class Mul(Instruction):
    """
    Multiplies *op1* with *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """

@instruction("int.div", op1=Integer, op2=Integer, target=Integer, callback=_unifyConstants)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result. Operands and target must be
    of same width.  If the product overflows the range of the integer type,
    the result in undefined. 
    
    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    
@instruction("int.eq", op1=Integer, op2=Integer, target=Bool, callback=_unifyConstants)
class Eq(Instruction):
    """
    Returns true iff *op1* equals *op2*. 
    """
    
@instruction("int.lt", op1=Integer, op2=Integer, target=Bool, callback=_unifyConstants)
class Lt(Instruction):
    """
    Returns true iff *op1* is less than *op2*.
    """

@instruction("int.ext", op1=Integer, target=Integer, callback=_unifyConstants)
class Ext(Instruction):
    """
    Zero-extends *op1* into an integer of the same width as the *target*. The
    width of *op1* must be smaller or equal that of the *target*. 
    """

@instruction("int.trunc", op1=Integer, target=Integer, callback=_unifyConstants)
class Trunc(Instruction):
    """
    Bit-truncates *op1* into an integer of the same width as the *target*. The
    width of *op1* must be larger or equal that of the *target*. 
    """


        

# $Id$
"""
Doubles
~~~~~~~
"""

_doc_type_description = """
The +double+ data type represents a 64-bit floating-point numbers.
If not explictly initialized, doubles are set to zero initially.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *

@instruction("double.add", op1=double, op2=double, target=double)
class Add(Instruction):
    """
    Calculates the sum of the two operands. If the sum overflows the range of
    the double type, the result in undefined. 
    """

@instruction("double.sub", op1=double, op2=double, target=double)
class Sub(Instruction):
    """
    Subtracts *op1* one from *op2*. If the difference underflows the range of
    the double type, the result in undefined. 
    """
    
@instruction("double.mul", op1=double, op2=double, target=double)
class Mul(Instruction):
    """
    Multiplies *op1* with *op2*. If the product overflows the range of the
    double type, the result in undefined. 
    """

@instruction("double.div", op1=double, op2=nonZero(double), target=double)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result.     
    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    
@instruction("double.eq", op1=double, op2=double, target=bool)
class Eq(Instruction):
    """
    Returns true iff *op1* equals *op2*. 
    """
    
@instruction("double.lt", op1=double, op2=double, target=bool)
class Lt(Instruction):
    """
    Returns true iff *op1* is less than *op2*.
    """

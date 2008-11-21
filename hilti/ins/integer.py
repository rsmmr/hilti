# $Id$
#
# Integer data type.

from instruction import *
from type import *

@instruction("integer.add", op1=Integer, op2=Integer, target=Integer)
class Add(Instruction):
    """
    Calculates sum of two operands. Operands and target must be of same type.
    """

@instruction("integer.sub", op1=Integer, op2=Integer, target=Integer)
class Sub(Instruction):
    """
    Subtracts *op1* one from *op2*. Operands and target must be of same type.
    """
    
@instruction("integer.mult", op1=Integer, op2=Integer, target=Integer)
class Mult(Instruction):
    """
    Multiplies *op1* with *op2*. Operands and target must be of same type.
    """

@instruction("integer.div", op1=Integer, op2=Integer, target=Integer)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result. Operands and target must be of same type. 
    
    Throws *Exception.DivisionByZero* if *op2* is zero.
    """
    
        

        

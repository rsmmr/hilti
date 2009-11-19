# $Id$
#
# The signed integer type.

from binpac.core import expr
from binpac.core.type import *
from binpac.core.operators import * 

@operator.Plus(UnsignedInteger, UnsignedInteger)
class Plus:
    def type(expr1, expr2):
        return UnsignedInteger(max(expr1.type().width(), expr2.type().width()))
    
    def fold(expr1, expr2):
        expr1 = expr1.fold()
        expr2 = expr2.fold()
    
        if isinstance(expr1, expr.Constant) and isinstance(expr2, expr.Constant):
            val = expr1.constant().value() + expr2.constant().value()
            const  = constant.Constant(val, UnsignedInteger(max(expr1.type().width(), expr2.type().width())))
            return expr.Constant(const)

        else:
            return self
        
    def codegen(expr1, expr2):
        util.internal_error("not implemented")


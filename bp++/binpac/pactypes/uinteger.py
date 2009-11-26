# $Id$
#
# The unsigned integer type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator

import hilti.core.type

@type.pac
class UnsignedInteger(type.Integer):
    """Type for unsigned integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(UnsignedInteger, self).__init__(width, location=location)

    def name(self):
        # Overridden from Type.
        return "uint<%d>" % self.width()
    
    def toCode(self):
        # Overridden from Type.
        return self.name()

    def hiltiType(self, cg, tag):
        return hilti.core.type.Integer(self.width() + 1)

    #def production(self):
    #    # XXX
    #    pass
    
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


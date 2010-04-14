# $Id$
#
# The unsigned integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("uint")
class UnsignedInteger(type.Integer):
    """Type for unsigned integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(UnsignedInteger, self).__init__(width, location=location)

    ### Overridden from Type.
    
    def name(self):
        return "uint%d" % self.width()

    def hiltiType(self, cg):
        return hilti.type.Integer(self.width())

    ### Overridden from ParseableType.

    def production(self, field):
        # XXX
        pass
    
    def generateParser(self, cg, dst):
        pass

@operator.Cast(type.Integer)
class Cast:
    def castConstantTo(const, dsttype):
        if isinstance(dsttype, type.UInteger) and const.value() >= 0:
            return constant.Constante(const.value(), type.Integer(dsttype.width()))
        
        raise operators.CastError
    
@operator.Plus(UnsignedInteger, UnsignedInteger)
class Plus:
    def type(expr1, expr2):
        return UnsignedInteger(max(expr1.type().width(), expr2.type().width()))
    
    def simplify(expr1, expr2):
        if expr1.isConst() and expr2.isConst():
            val = expr1.constant().value() + expr2.constant().value()
            const  = constant.Constant(val, UnsignedInteger(max(expr1.type().width(), expr2.type().width())))
            return expr.Constant(const)

        else:
            return self
        
    def evaluate(cg, expr1, expr2):
        util.internal_error("not implemented")


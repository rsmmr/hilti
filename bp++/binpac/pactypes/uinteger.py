# $Id$
#
# The unsigned integer type.

import integer

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator

import hilti.core.type

@type.pac("uint")
class UnsignedInteger(type.Integer):
    """Type for unsigned integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    attrs: list of (name, value) pairs - See ~~ParseableType.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, attrs=[], location=None):
        super(UnsignedInteger, self).__init__(width, attrs=attrs, location=location)

    ### Overridden from Type.
    
    def name(self):
        return "uint%d" % self.width()

    def validate(self, vld):
        pass
    
    def hiltiType(self, cg, tag):
        return hilti.core.type.Integer(self.width())

    def toCode(self):
        return self.name()
    
    ### Overridden from ParseableType.

    def production(self):
        # XXX
        pass
    
    def generateParser(self, codegen, dst):
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
    
    def fold(expr1, expr2):
        expr1 = expr1.fold()
        expr2 = expr2.fold()
    
        if expr1 and expr2:
            val = expr1.constant().value() + expr2.constant().value()
            const  = constant.Constant(val, UnsignedInteger(max(expr1.type().width(), expr2.type().width())))
            return expr.Constant(const)

        else:
            return self
        
    def evaluate(codegen, builder, expr1, expr2):
        util.internal_error("not implemented")


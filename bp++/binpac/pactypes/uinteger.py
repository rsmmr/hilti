# $Id$
#
# The unsigned integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.constant as constant
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

    def hiltiCtor(self, cg, ctor):
        const = hilti.constant.Constant(ctor, hilti.type.Integer(self.width()))
        return hilti.operand.Constant(const)
    
    ### Overridden from ParseableType.

    def production(self, field):
        # XXX
        pass
    
    def generateParser(self, cg, dst):
        pass

@operator.Coerce(type.UnsignedInteger)
class Coerce:
    def coerceCtorTo(ctor, dsttype):
        if isinstance(dsttype, type.Bool):
            return ctor != 0
        
        if isinstance(dsttype, type.SignedInteger):
            return ctor 
        
        raise operator.CoerceError

    def canCoerceExprTo(expr, dsttype):
        if isinstance(dsttype, type.Integer):
            return True
        
        if isinstance(dsttype, type.Bool):
            return True
        
        return False
        
    def coerceExprTo(cg, e, dsttype):
        if isinstance(dsttype, type.Bool):
            tmp = cg.builder().addLocal("__nonempty", hilti.type.Bool())
            cg.builder().equal(tmp, e.evaluate(cg), cg.builder().constOp(0))
            cg.builder().bool_not(tmp, tmp)
            return expr.Hilti(tmp, type.Bool())
        
        if dsttype.width() >= e.type().width():
            return e
        
        tmp = cg.builder().addLocal("__truncated", dsttype)
        cg.builder().int_trunc(tmp, e.evaluate(cg))
        return expr.Hilti(tmp, dsttype)
    
@operator.Plus(UnsignedInteger, UnsignedInteger)
class Plus:
    def type(expr1, expr2):
        return UnsignedInteger(max(expr1.type().width(), expr2.type().width()))
    
    def simplify(expr1, expr2):
        if isinstanct(expr1, expr.Ctor) and isinstanct(expr2, expr.Ctor):
            val = expr1.value() + expr2.value()
            return expr.Ctor(val, UnsignedInteger(max(expr1.type().width(), expr2.type().width())))

        else:
            return None
        
    def evaluate(cg, expr1, expr2):
        util.internal_error("not implemented")


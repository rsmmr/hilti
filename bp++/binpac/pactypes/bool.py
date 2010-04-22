# $Id$
#
# The bytes type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator
import binpac.constant as constant

import hilti.type

@type.pac("bool")
class Bool(type.ParseableType):
    """Type for bool objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Bool, self).__init__(location=location)

    ### Overridden from Type.
    
    def validate(self, vld):
        return True

    def validateConst(self, vld, const):
        if not isinstance(const.value(), bool):
            vld.error(const, "bool: constant of wrong internal type")
            
    def hiltiType(self, cg):
        return hilti.type.Bool()

    def name(self):
        return "bool" 
    
    def pac(self, printer):
        printer.output("bool")
        
    def pacConstant(self, printer, value):
        printer.output("True" if value else "False")
    
    ### Overridden from ParseableType.
    
    def supportedAttributes(self):
        return {}

    def production(self, field):
        util.internal_error("bool parsing not implemented")
    
    def generateParser(self, cg, cur, dst, skipping):
        util.internal_error("bool parsing not implemented")
        
@operator.Not(Bool)
class _:
    def type(e):
        return type.Bool()
    
    def simplify(e):
        if not e.isConst():
            return None
        
        inverse = not e.constant().value()
        return expr.Constant(constant.Constant(inverse, type.Bool()))
        
    def evaluate(cg, e):
        tmp = cg.functionBuilder().addLocal("__not", hilti.type.Bool())
        cg.builder().bool_not(tmp, e.evaluate(cg))
        return tmp
        
@operator.And(Bool, Bool)
class _:
    def type(e1, e2):
        return type.Bool()
    
    def simplify(e1, e2):
        if not (e1.isConst() and e2.isConst()):
            return None
        
        result = e1.constant().value() and e2.constant().value()
        return expr.Constant(constant.Constant(result, type.Bool()))
        
    def evaluate(cg, e1, e2):
        tmp = cg.functionBuilder().addLocal("__and", hilti.type.Bool())
        cg.builder().bool_and(tmp, e1.evaluate(cg), e2.evaluate(cg))
        return tmp    
    
    
        
    

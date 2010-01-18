# $Id$
#
# The bytes type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator
import binpac.core.constant as constant

import hilti.core.type

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
        return hilti.core.type.Bool()

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
    
    def fold(e):
        if not e.isConst():
            return None
        
        inverse = not e.fold().constant().value()
        return expr.Constant(constant.Constant(inverse, type.Bool()))
        
    def evaluate(cg, e):
        tmp = cg.functionBuilder().addTmp("__not", hilti.core.type.Bool())
        cg.builder().bool_not(tmp, e.evaluate(cg))
        return tmp
        
    
    
        
    

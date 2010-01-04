# $Id$
#
# The bytes type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator

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
            vld.error(const, "constant of wrong internal type")
            
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

    def production(self):
        util.internal_error("bool parsing not implemented")
    
    def generateParser(self, cg, cur, dst, skipping):
        util.internal_error("bool parsing not implemented")
        
        
        
    
    
        
    

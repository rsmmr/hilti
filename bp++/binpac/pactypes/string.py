# $Id$
#
# The bytes type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator

import hilti.core.type

@type.pac("string")
class String(type.ParseableType):
    """Type for string objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(String, self).__init__(location=location)

    ### Overridden from Type.
    
    def name(self):
        return "string" 

    def validate(self, vld):
        return True

    def validateConst(self, vld, const):
        if not isinstance(const.value(), str):
            vld.error(const, "string: constant of wrong internal type")
            
    def hiltiType(self, cg):
        return hilti.core.type.String()

    def pac(self, printer):
        printer.output("string")
        
    def pacConstant(self, printer, const):
        printer.output("\"%s\"" % const.value())
    
    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {}

    def production(self):
        util.internal_error("string parsing not implemented")
    
    def generateParser(self, cg, cur, dst, skipping):
        util.internal_error("string parsing not implemented")
        
        
        
    
    
        
    

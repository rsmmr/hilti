# $Id$
#
# The bytes type.

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

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
        if not isinstance(const.value(), str) and not isinstance(const.value(), unicode):
            vld.error(const, "string: constant of wrong internal type")
            
    def hiltiType(self, cg):
        return hilti.type.String()

    def pac(self, printer):
        printer.output("string")
        
    def pacConstant(self, printer, const):
        printer.output("\"%s\"" % const.value())
    
    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return {}

    def production(self, field):
        util.internal_error("string parsing not implemented")
    
    def generateParser(self, cg, cur, dst, skipping):
        util.internal_error("string parsing not implemented")
        
        
        
    
    
        
    

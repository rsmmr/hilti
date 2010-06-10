# $Id$
#
# The void type.

import binpac.type as type

import hilti.type

@type.pac("void")
class Void(type.Type):
    """Void type.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Void, self).__init__(location=location)

    ### Overridden from Type.
    
    def hiltiType(self, cg):
        return hilti.type.Void()

    def name(self):
        return "void" 
    
    def pac(self, printer):
        printer.output("void")
        

# $Id$
#
# The signed integer type.

import integer

import binpac.type as type
import binpac.expr as expr
import binpac.operator as operator

import hilti.type

@type.pac("int")
class SignedInteger(type.Integer):
    """Type for signed integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(SignedInteger, self).__init__(width, location=location)

        
    ### Overridden from Type.
    
    def name(self):
        return "int%d" % self.width()

    def hiltiType(self, cg):
        return hilti.type.Integer(self.width())

    ### Overridden from ParseableType.

    def production(self, field):
        # XXX
        pass
    
    def generateParser(self, cg, dst):
        pass
        

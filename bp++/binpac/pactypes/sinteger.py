# $Id$
#
# The signed integer type.

import integer

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator

import hilti.core.type

@type.pac("int")
class SignedInteger(type.Integer):
    """Type for signed integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    attrs: list of (name, value) pairs - See ~~ParseableType.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, attrs=[], location=None):
        super(SignedInteger, self).__init__(width, attrs=attrs, location=location)

        
    ### Overridden from Type.
    
    def name(self):
        return "int%d" % self.width()

    def hiltiType(self, cg):
        return hilti.core.type.Integer(self.width())

    ### Overridden from ParseableType.

    def production(self):
        # XXX
        pass
    
    def generateParser(self, cg, dst):
        pass
        

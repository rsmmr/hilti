# $Id$
#
# The signed integer type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.operator as operator

import hilti.core.type

@type.pac
class SignedInteger(type.Integer):
    """Type for signed integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.

    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(SignedInteger, self).__init__(width, location=location)

    def name(self):
        # Overridden from Type.
        return "int<%d>" % self.width()
    
    def toCode(self):
        # Overridden from Type.
        return self.name()

    def hiltiType(self, cg, tag):
        return hilti.core.type.Integer(self.width())

    #def production(self):
    #    # XXX
    #    pass
    

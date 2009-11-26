# $Id$
#
# The bytes type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.grammar as grammar
import binpac.core.operator as operator

import hilti.core.type

@type.pac
class Bytes(type.Type):
    """Type for bytes objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(Bytes, self).__init__(prod=True, location=location)

    def name(self):
        # Overridden from Type.
        return "bytes" 
    
    def toCode(self):
        # Overridden from Type.
        return self.name()

    def hiltiType(self, cg, tag):
        return hilti.core.type.Bytes()

    def production(self):
        return grammar.Variable(None, hilti.core.type.Bytes(), location=self.location())
        
    

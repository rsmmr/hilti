# $Id$
#
# The regexp type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.grammar as grammar
import binpac.core.operator as operator

import hilti.core.type

@type.pac
class RegExp(type.Type):
    """Type for regexp objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(RegExp, self).__init__(prod=True, location=location)

    def name(self):
        # Overridden from Type.
        return "regexp" 
    
    def toCode(self):
        # Overridden from Type.
        return self.name()

    def hiltiType(self, cg, tag):
        return hilti.core.type.RegExp()

    def production(self):
        return grammar.Variable(None, hilti.core.type.RegExp(), location=self.location())
        
    

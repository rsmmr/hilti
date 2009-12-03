# $Id$
#
# The regexp type.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.grammar as grammar
import binpac.core.operator as operator

import hilti.core.type

@type.pac("regexp")
class RegExp(type.ParseableType):
    """Type for regexp objects.  
    
    attrs: list of (name, value) pairs - See ~~ParseableType.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, attrs=[], location=None):
        super(RegExp, self).__init__(attrs=attrs, location=location)

    def toCode(self):
        # Overridden from Type.
        return self.name()

    def hiltiType(self, cg, tag):
        return hilti.core.type.RegExp()

    def production(self):
        return grammar.Variable(None, hilti.core.type.RegExp(), location=self.location())
        
    

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
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(RegExp, self).__init__(location=location)

    def hiltiType(self, cg):
        return hilti.core.type.Reference([hilti.core.type.Bytes()])

    def production(self, field):
        return grammar.Variable(None, hilti.core.type.RegExp(), location=self.location())
        
    def validateConst(self, vld, const):
        if not isinstance(const.value(), str):
            vld.error(const, "regular expression: constant of wrong internal type")
    
    def pac(self, printer):
        printer.output("regexp")
        
    def pacConstant(self, printer, const):
        printer.output("/%s/" % const.value())
    

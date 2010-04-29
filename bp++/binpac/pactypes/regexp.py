# $Id$
#
# The regexp type.

import binpac.type as type
import binpac.expr as expr
import binpac.grammar as grammar
import binpac.operator as operator

import hilti.type

@type.pac("regexp")
class RegExp(type.ParseableType):
    """Type for regexp objects.  
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, location=None):
        super(RegExp, self).__init__(location=location)

    def parsedType(self):
        return type.Bytes()
        
    def hiltiCtor(self, cg, ctor):
        return hilti.operand.Ctor(([ctor], []), hilti.type.Reference(hilti.type.RegExp()))
            
    def hiltiType(self, cg):
        return hilti.type.Reference(hilti.type.Bytes())

    def validateCtor(self, vld, ctor):
        if not isinstance(ctor, str):
            vld.error(ctor, "regular expression: constant of wrong internal type")
    
    def pac(self, printer):
        printer.output("regexp")
        
    def pacCtor(self, printer, ctor):
        printer.output("/%s/" % ctor)
    
    ### Overridden from ParseableType.

    def supportedAttributes(self):
        return { 
            "default": (self, True, None),
            "convert": (type.Any(), False, None),
            }
    
    def production(self, field):
        return grammar.Variable(None, hilti.type.RegExp(), location=self.location())
        
    

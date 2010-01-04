# $Id$
#
# Base class for integer types.

import binpac.core.type as type
import binpac.core.expr as expr
import binpac.core.grammar as grammar
import binpac.core.operator as operator

import hilti.core.type

@type.pac("int")
class Integer(type.ParseableType):
    """Base class for integers.  
    
    width: integer - Specifies the bit-width of integers represented by this type.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, location=None):
        super(Integer, self).__init__(location=location)
        assert(width > 0 and width <= 64)
        self._width = width

    def width(self):
        """Returns the bit-width of the type's integers.
        
        Returns: int - The number of bits available to represent integers of
        this type. 
        """
        return self._width

    def setWidth(self, width):
        """Sets the bit-width of the type's integers.
        
        width: int - The new bit-width. 
        """
        self._width = width

    ### Overridden from Type.
    
    def validate(self, vld):
        if self._width < 1 or self._width > 64:
            vld.error(self, "integer width out of range")

    def validateConst(self, vld, const):
        if not isinstance(const.value(), int):
            vld.error(const, "constant of wrong internal type")
            
    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.width() == other.width()
        
        return NotImplemented
    
    def pac(self, printer):
        printer.output(self.name())
    
    def pacConstant(self, printer, const):
        printer.output("%d" % const.value())
    
    ### Overridden from ParseableType.
    
    def supportedAttributes(self):
        return {}

        
        
        
        
        

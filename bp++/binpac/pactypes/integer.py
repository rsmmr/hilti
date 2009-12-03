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
    
    attrs: list of (name, value) pairs - See ~~ParseableType.
    
    location: ~~Location - A location object describing the point of definition.
    """
    def __init__(self, width, attrs=[], location=None):
        super(Integer, self).__init__(attrs=attrs, location=location)
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

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.width() == other.width()
        
        return NotImplemented
            
    ### Overridden from ParseableType.
    
    def supportedAttributes(self):
        return {}

        
        
        
        
        

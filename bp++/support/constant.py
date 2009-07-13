# $Id$
# 
# A constant.

import ast
import location

class Constant(object):
    """Represents a constant value with its type.
    
    value: any - The constant's value in accordance with *type*
    type: ~~Type -- The constant's type.
    location: ~~Location - A location to be associated with the constant. 
    
    Note: The class maps the ~~Visitor subtype to :meth:`~type`.
    """
    def __init__(self, value, type, location=None):
        self._value = value
        self._type = type
        self._location = location
        
    def value(self):
        """Returns the value of the constant.
        
        Returns: any - Value of the constant.
        """
        return self._value
    
    def type(self):
        """Returns the type of the constant.
        
        Returns: ~~Type - Type of the constant.
        """
        return self._type

    def setType(self, type):
        """Sets the type of the constant.
        
        type: ~~Type - The type to set.
        """
        self._type = type

    def location(self):
        """Returns the location associated with the constant.
        
        Returns: ~~Location - The location. 
        """
        return self._location
        
    def __str__(self):
        return "%s: %s" % (self._value, self._type)
    
    # Visitor support.
    def visitorSubType(self):
        return self._type


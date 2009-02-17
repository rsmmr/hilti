# $Id$
# 
# A constant.

import ast
import location
import type

class Constant(ast.Node):
    """A Constant represents a constant value with its
    ~~Type.
    
    The class maps the ~~Visitor subtype to :meth:`~type`.
    """
    def __init__(self, value, type, location = None):
        """Initializes the constant with *value*, which must match with the
        ~~Type *type*. *location* defines a
        ~~Location to be associated with this
        Constant."""
        self._value = value
        self._type = type
        self._location = location
        
    def value(self):
        """Return the value of the Constant."""
        return self._value
    
    def type(self):
        """Return the ~~Type of the Constant."""
        return self._type

    def setType(self, type):
        """Sets the ~~Type of the Constant to *type*."""
        self._type = type
    
    def location(self):
        """Returns the ~~Location associated with
        the Constant."""
        return self._location

    def __str__(self):
        return " %s %s" % (self._type, self._value)
    
    # Visitor support.
    def visitorSubType(self):
        return self._type

Void = Constant("void", type.Void)
"""The special Constant representing a *Void* value."""
    

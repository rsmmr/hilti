# $Id$
# 
# A constant.

import location
import type

class Constant(object):
    def __init__(self, value, type, location = location.Location()):
        self._value = value
        self._type = type
        self._location = location
        
    def value(self):
        return self._value
    
    def type(self):
        return self._type

    def setType(self, type):
        self._type = type
    
    def location(self):
        return self._location
    
        # Visitor support.
    def dispatch(self, visitor):
        visitor.visit(self)
        
    def visitorSubClass(self):
        return self._type
    
    def __str__(self):
        return " %s %s" % (self._type, self._value)

Void = Constant("void", type.Void)
    

# $Id$
#
# An ID, which binds a name to a type.

import location

class ID(object):
    def __init__(self, name, type, location = location.Location()):
        self._name = name
        self._type = type
        self._location = location

    def name(self):
        return self._name

    def type(self):
        return self._type
    
    def location(self):
        return self._location

    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit(self)
        
    def visitorSubClass(self):
        return self._type
    
    def __str__(self):
        location = " [%s]" % self._location if self._location else ""
        return "%s:%s%s" % (self._name, self._type, location)


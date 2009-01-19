# $Id$
#
# A module.

import id
import location
import scope

class Module(object):
    def __init__(self, name, location = location.Location()):
        self._name = name
        self._location = location
        self._scope = scope.Scope()
        self._functions = {}

    def name(self):
        return self._name

    def location(self):
        return self._location
    
    def scope(self):
        return self._scope

    def function(self, name):
        try:
            return self._functions[name]
        except LookupError:
            return None
    
    def addID(self, id):
        self._scope.insert(id)

    def addFunction(self, function):
        self.addID(id.ID(function.name(), function.type()))
        self._functions[function.name()] = function
        
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        
        self._scope.dispatch(visitor)
        for func in self._functions.values():
            visitor.dispatch(func)
        
        visitor.visit_post(self)
        
    def __str__(self):
        return "module %s" % self._name
    
        
    

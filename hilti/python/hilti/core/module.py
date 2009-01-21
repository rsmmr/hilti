# $Id$
#
# A module.

import id
import location
import scope

class Module(object):
    def __init__(self, name, location = location.Location()):
        self._name = name.lower()
        self._location = location
        self._scope = scope.Scope()
        self._globals = {}

    def name(self):
        return self._name

    def location(self):
        return self._location
    
    def IDs(self):
        return self._scope.IDs()

    def _canonalizeScope(self, name):
        # Lower-case module name if it has one.
        i = name.find("::") 
        if i >= 0:
            name = "%s::%s" % (name[0:i].lower(), name[i+2:])
        return name
    
    def addGlobal(self, id, value = True):
        # must be qualified
        name = id.name()
        assert name.find("::") >= 0
        name = self._canonalizeScope(name)
        id.setName(name)
        self._globals[name] = value
        self._scope.insert(id)

    def lookupID(self, name):
        i = name.find("::") 
        if i < 0:
            # Qualify if it isn't.
            name = "%s::%s" (self._name(), name)

        name = self._canonalizeScope(name)
            
        return self._scope.lookup(name)
        
    def lookupGlobal(self, name):
        i = name.find("::") 
        if i < 0:
            # Qualify if it isn't.
            name = "%s::%s" (self._name(), name)
        
        name = self._canonalizeScope(name)
        
        try:
            return self._globals[name]
        except LookupError:
            return None
        
        
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        
        self._scope.dispatch(visitor)
        for func in self._globals.values():
            visitor.dispatch(func)
        
        visitor.visit_post(self)
        
    def __str__(self):
        return "module %s" % self._name
    
        
    

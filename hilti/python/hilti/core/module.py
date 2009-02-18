# $Id$

import ast
import id as idmod
import location
import type
import visitor

class Module(ast.Node):
    """A Module represents a single HILTI link-time unit. Each Module has a
    scope of identifier defining which ~~ID objects are visible inside the
    ~~Module.
    
    *name* is string containing the globally visible name of the Module; names
    are considered case-insensitive. *location* associates a ~~Location with
    the Module. 
    
    When using a ~~Visitor with a Module, it will traverse all IDs,
    and their *values*, in the Module's scope (see :meth:`addID`).
    """
    
    def __init__(self, name, location=None):
        self._name = name.lower()
        self._location = location
        self._scope = {}

    def name(self):
        """Returns the globally visible name of the Module as a string."""
        return self._name

    def location(self):
        """Returns the ~~Location object associated with the Module."""
        return self._location
    
    def IDs(self):
        """Returns a list of all ~~ID objects that are part of the Module's
        scope."""
        return [id for (id, value) in self._scope.values()]

    def _canonName(self, id):
        (scope, name) = id.splitScope()
        if not scope or scope == self._name.lower():
            scope = "<local>"
        
        return "%s::%s" % (scope, name)
    
    def addID(self, id, value = True):
        """Adds an ~~ID to the Module's scope and associates an arbitrary
        *value* with it. If there's no specific value that needs to be stored
        with the ID, use the default of *True*.
        
        An ~~ID defined elsewhere can be imported into a Module by adding it 
        with a fully qualified name (i.e., +<ext-module>::<name>+). In this
        case, subsequent lookups will only succeed if they are likewise fully
        qualified. If *id* comes with a fully-qualified name that matches the
        Modules'name (as returned by :meth:`~hilti.name`), subsequent lookups
        will succeed no matter whether they are qualified or not. 
        """
        idx = self._canonName(id)
        self._scope[idx] = (id, value)

    def lookupID(self, name):
        """Returns the value associated with the ~~ID of the given *name* if
        it exists in the Module scope, and *None* otherwise."""

        # Need the tmp just for the name splitting. 
        tmp = idmod.ID(name, type.Type, 0) 
        idx = self._canonName(tmp)
        
        try:
            (id, value) = self._scope[idx]
            return id
        
        except KeyError:
            return None

    def lookupIDVal(self, name):
        """Returns the value associated with the ~~ID of the given *name* if
        it exists in the Module scope, and *None* otherwise."""

        # Need the tmp just for the name splitting. 
        tmp = idmod.ID(name, type.Type, 0) 
        idx = self._canonName(tmp)
        
        try:
            (id, value) = self._scope[idx]
            return value
        
        except KeyError:
            return None
        
    def __str__(self):
        return "module %s" % self._name
    
    # Visitor support.
    def visit(self, v):
        v.visitPre(self)
        
        for (id, value) in self._scope.values():
            v.visit(id)
            if isinstance(value, visitor.Visitable):
                v.visit(value)
        
        v.visitPost(self)
        
    
        
    

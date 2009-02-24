# $Id$

import ast
import id as idmod
import location
import type
import visitor

class Module(ast.Node):
    """Represents a single HILTI link-time unit. A module has its
    own identifier scope defining which ~~ID objects are visibile
    inside its namespace.  
    
    name: string - The globally visible name of the module; modules
    names are treated case-insensitive. 
    
    location: ~~Location - A location to be associated with the function. 

    Note: When traversion a module with a ~~Visitor, it will visit all ~~ID
    objects in its scope *and* all their values (see :meth:`addID`)
    """
    def __init__(self, name, location=None):
        super(Module, self).__init__(location)
        self._name = name.lower()
        self._location = location
        self._scope = {}

    def name(self):
        """Returns the name of the module.
        
        Returns: string - The name.
        """
        return self._name

    def IDs(self):
        """Returns all IDs in the module's scope. 
        
        Returns: list of ~~ID - The IDs. 
        """
        return [id for (id, value) in self._scope.values()]

    def _canonName(self, id):
        (scope, name) = id.splitScope()
        if not scope or scope == self._name.lower():
            scope = "<local>"
        
        return "%s::%s" % (scope, name)
    
    def addID(self, id, value = True):
        """Adds an ID to the module's scope. An arbitrary value can be
        associated with each ~~ID. If there's no specific value that needs to
        be stored, just use the default of *True*.
        
        An ~~ID defined elsewhere can be imported into a module by adding it 
        with a fully qualified name (i.e., ``<ext-module>::<name>``). In this
        case, subsequent lookups will only succeed if they are likewise fully
        qualified. If ~~ID comes with a fully-qualified name that matches the
        module's own name, subsequent lookups will succeed no matter whether
        they are qualified or not. 
        
        id: ~~ID - The ID to add to the module's scope.
        value: any - The value to associate with the ID.
        """
        idx = self._canonName(id)
        self._scope[idx] = (id, value)

    def lookupID(self, name):
        """Looks up an ID in the module's scope. 
        
        name: string - The name of the ID to lookup (see :meth:`addID` for the
        lookup rules used for qualified names).
        
        Returns: ~~ID - The ID, or None if the name is not found.
        """
        # Need the tmp just for the name splitting. 
        tmp = idmod.ID(name, type.Type, 0) 
        idx = self._canonName(tmp)
        
        try:
            (id, value) = self._scope[idx]
            return id
        
        except KeyError:
            return None

    def lookupIDVal(self, name):
        """Looks up the value associated with an ID in the module's scope. 
        
        name: string - The name of the ID to lookup (see :meth:`addID` for the
        lookup rules used for qualified names).
        
        Returns: any - The value associated with the ID, or None if the name
        is not found.
        """
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
        
    
        
    

# $Id$

import ast
import id as idmod
import location
import type
import visitor

import sys

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
        """Returns the name of the module. The module's name will have been
        converted to all lower-case in accordance with the policy to treat is
        case-independent.
        
        Returns: string - The name.
        """
        return self._name

    def IDs(self):
        """Returns all IDs in the module's scope. 
        
        Returns: list of ~~ID - The IDs. 
        """
        return [id for (id, value) in self._scope.values()]

    def _canonName(self, scope, name):
        if not scope:
            scope = self._name
            
        return "%s~%s" % (scope, name)
    
    def addID(self, id, value = True):
        """Adds an ID to the module's scope. An arbitrary value can be
        associated with each ~~ID. If there's no specific value that needs to
        be stored, just use the default of *True*.
        
        An ~~ID defined elsewhere can be imported into a module by adding it
        with a scope of that other module. In this case, subsequent lookups
        will only succeed if they are accordingly qualified
        (``<ext-module>::<name>``). If the ID comes either without a scope,
        or with a scope that matches the module's own name, subsequent lookups
        will succeed no matter whether they are qualified or not. 
        
        If the ID does not have a scope, we set it's scope to the name of the
        module.
        
        id: ~~ID - The ID to add to the module's scope.
        value: any - The value to associate with the ID.
        with a module namespace even if there's a scope separator in there. 
        """
        
        idx = self._canonName(id.scope(), id.name())
        self._scope[idx] = (id, value)
        
        if not id.scope():
            id.setScope(self._name)

    def _lookupID(self, id, return_val):

        if isinstance(id, str):
            # We first look it up as a module-local variable, assuming there's no
            # scope in the name. 
            try:
                idx = self._canonName(None, id)
                (id, value) = self._scope[idx]
                return value if return_val else id
            except KeyError:
                pass
        
            # Now see if there's a scope given.
            i = id.find("::")
            if i < 0:
                # No scope.
                return None

            # Look up with the scope.
            scope = id[0:i].lower()
            name = id[i+2:]
            
        else:
            assert isinstance(id, idmod.ID)
            scope = id.scope()
            name = id.name()
            
        idx = self._canonName(scope, name)
        
        try:
            (id, value) = self._scope[idx]
            return value if return_val else id
        
        except KeyError:
            return None        
        
    def lookupID(self, id):
        """Looks up an ID in the module's scope. 
        
        id: string or ~~ID - Either the name of the ID to lookup (see
        :meth:`addID` for the lookup rules used for qualified names), or the
        ~~ID itself. The latter is useful to check whether the ID is part of
        the module's scope.
        
        Returns: ~~ID - The ID, or None if the name is not found.
        """
        return self._lookupID(id, False)

    def lookupIDVal(self, id):
        """Looks up the value associated with an ID in the module's scope. 
        
        id: string or ~~ID - Either the name of the ID to lookup (see
        :meth:`addID` for the lookup rules used for qualified names), or the
        ~~ID itself.
        
        Returns: any - The value associated with the ID, or None if the name
        is not found.
        """
        return self._lookupID(id, True)

    def __str__(self):
        return "module %s" % self._name
    
    # Visitor support.
    def visit(self, v):
        v.visitPre(self)
        
        for (id, value) in sorted(self._scope.values()):
            v.visit(id)
            if isinstance(value, visitor.Visitable):
                v.visit(value)
        
        v.visitPost(self)
        
    
        
    

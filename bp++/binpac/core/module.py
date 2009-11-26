# $Id$

builtin_id = id

import ast
import id
import type
import location
import binpac.support.visitor as visitor

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
        self._imported_modules = [] # Set by the parser.

    def name(self):
        """Returns the name of the module. The module's name will
        have been converted to all lower-case in accordance with the
        policy to treat it case-independent.
        
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
        be stored, just use the default of *True*. If the ID already exists,
        the old entry is replace with the new one. 
        
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

    def _lookupID(self, i, return_val):

        if isinstance(i, str):
            # We first look it up as a module-local variable, assuming there's no
            # scope in the name. 
            try:
                idx = self._canonName(None, i)
                (i, value) = self._scope[idx]
                return value if return_val else i
            except KeyError:
                pass
        
            # Now see if there's a scope given.
            i = i.find("::")
            if i < 0:
                # No scope.
                return None

            # Look up with the scope.
            scope = i[0:i].lower()
            name = i[i+2:]
            
        else:
            assert isinstance(i, id.ID)
            scope = i.scope()
            name = i.name()
            
        idx = self._canonName(scope, name)
        
        try:
            (i, value) = self._scope[idx]
            return value if return_val else i
        
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

    def importedModules(self):
        """Returns the modules which have been imported into this modules
        namespace. 
        
        Returns: list of (module, path) - List of imported modules. ``module``
        is the name of the module as it specified to be imported, and ``path``
        is the full path of the file that got imported."""
        return self._imported_modules

    def importIDs(self, other):
        """Makes the IDs of another module available to the current one.
        
        other: ~~Module - The other module.
        """
    
        for i in other.IDs():
        
            if i.imported():
                # Don't import IDs recursively.
                continue
            
            t = i.type()
            
            if i.linkage() == id.Linkage.EXPORTED:
                val = self.lookupIDVal(i)
                newid = id.ID(i.name(), i.type(), i.role(), i.linkage, scope=i.scope(), imported=True, location=id.location())
                self.addID(newid, val)
                
            # FIXME: We should introduce linkage for all IDs so that we can copy
            # only "exported" ones over.
            if isinstance(t, type.TypeDecl) or i.role() == id.Role.CONST:
                val = other.lookupIDVal(i)
                newid = id.ID(i.name(), i.type(), i.role(), scope=i.scope(), imported=True, location=i.location())
                self.addID(newid, val)
                continue
            
            # Cannot export types other than those above at the moment. 
            util.internal_error("can't handle IDs of type %s (role %d) in import" % (repr(t), i.role()))
    
    def __str__(self):
        return "module %s" % self._name
    
    # Visitor support.
    
    def _visitType(self, v, ids, visited, filter):
        
        objs = []
        
        for name in ids:
            if name in visited:
                continue
            
            (id, value) = self._scope[name]
            if not filter(id):
                continue
            
            objs += [(id, value)]
            visited += [name]

        for (id, value) in sorted(objs, key=lambda id: id[0].name()):
            v.visit(id)
            if isinstance(value, visitor.Visitable):
                v.visit(value)
            
    def visit(self, v):
        v.visitPre(self)

        # Sort the ID names so that we get a deterministic order. 
        ids = sorted(self._scope.keys())
        
        visited = []
        self._visitType(v, ids, visited, lambda i: isinstance(i.type(), type.TypeDecl))
        self._visitType(v, ids, visited, lambda i: i.role() == id.Role.CONST)
        self._visitType(v, ids, visited, lambda i: True) # all the rest.
        
        v.visitPost(self)
        
    
        
    

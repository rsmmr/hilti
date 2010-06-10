# $Id$

builtin_id = id

import node
import id
import scope
import type
import location
import binpac.visitor as visitor

class Module(node.Node):
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
        self._scope = scope.Scope(name, None)
        self._stmts = []
        self._imported_modules = [] # Set by the parser.

    def name(self):
        """Returns the name of the module. The module's name will
        have been converted to all lower-case in accordance with the
        policy to treat it case-independent.
        
        Returns: string - The name.
        """
        return self._name

    def scope(self):
        """Returns the module's scope. 
        
        Returns: ~~Scope - The scope. 
        """
        return self._scope
    
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
    
        for i in other._scope.IDs():
        
            if i.imported():
                # Don't import IDs recursively.
                continue
            
            t = i.type()
            
            if i.linkage() == id.Linkage.EXPORTED:
                newid = id.clone()
                newid.setImported()
                self._scope.addID(newid)
                continue
            
            # Cannot export types other than those above at the moment. 
            util.internal_error("can't handle IDs of type %s in import" % (repr(t)))

    def addStatement(self, stmt):
        """Adds a global statements to the module. Global statements will
        executed at module initialization time.
        
        stmt: ~~Statement - The statement.
        """
        self._stmts += [stmt]

    def statements(self):
        """Returns the module-global statements. These will be execute at
        module initialization time.
        
        Returns : list of ~~Statement - The statements.
        """
        return self._stmts
        
    def __str__(self):
        return "module %s" % self._name

    ### Overidden from node.Node.

    def validate(self, vld):
        for id in self._scope.IDs():
            id.validate(vld)
            
        for stmt in self.statements():
            stmt.validate(vld)

    def resolve(self, resolver):
        for id in self._scope.IDs():
            id.resolve(resolver)
            
        for stmt in self.statements():
            stmt.resolve(resolver)

    def pac(self, printer):
        printer.output("module %s\n" % self._name, nl=True)
        
        for id in self._scope.IDs():
            id.pac(printer)
            
        for stmt in self.statements():
            stmt.pac(printer)
        
    # Visitor support.
    def visit(self, v):
        v.visit(self._scope)
    
        
    

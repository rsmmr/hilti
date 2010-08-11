# $Id$

builtin_id = id

import node
import id
import scope
import type
import stmt
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
        self._hooks = []
        self._imported_modules = []

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
            
            newid = i.clone()
            newid.setLocation(i.location())
            newid.setImported()
            newid.setNamespace(other.name())
            self.scope().addID(newid)
                
        self._imported_modules += [(other, "<need to set path>")]

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
    
    def addExternalHook(self, ident, stmts, debug=False):
        """Adds an external hook to an existing unit. 
        
        ident: string - A string referencing the hook. The must be suitably
        qualified, e.g., ``MyUnit::my_hook`` or ``MyModule::MyUnit::my_hook``.
        
        stmts: ~~Block - The hook's body.

        debug: bool - If True, this hook will only compiled in if
        the code generator is including debug code, and it will only be executed
        if at run-time, debug mode is enabled (via the C function
        ~~binpac_enable_debug).
        """
        self._hooks += [(ident, stmts, debug)]
        
    def findUnitForHook(self, ident):
        """Locates the unit that an external hook identfier refers to. 
        
        ident: string - The suitably qualified name of hook.
        
        Returns: tuple (~~Unit, string) - If the identifier references a valid
        hook, the first element is the unit and the second the name of the
        field. If not, returns ``(None, None)``.
        """
        
        try:
            (unit, field) = ident.rsplit("::", 1)
        except ValueError:
            return (None, None)
        
        unit = self.scope().lookupID(unit)
        
        if not unit:
            return (None, None)
        
        if not isinstance(unit.type(), type.Unit):
            return (None, None)
        
        return (unit.type(), field)
        
    def __str__(self):
        return "module %s" % self._name

    ### Overidden from node.Node.

    def resolve(self, resolver):
        for id in self._scope.IDs():
            id.resolve(resolver)
            
        for s in self.statements():
            s.resolve(resolver)
            
        for (ident, stmts, debug) in self._hooks:
            stmts.resolve(resolver)
            
            (unit, field) = self.findUnitForHook(ident)
            if not unit:
                vld.error("%s does not reference a valid unit field/hook" % ident)
                continue
            
            # Add the hook to the unit, which will also take care of
            # validating it.
            hook = stmt.UnitHook(unit, None, 0, stmts=stmts, debug=debug)
            unit.addHook(field, hook, 0)
            # Trigger update of unit internal hook tracking.
            unit.doResolve(resolver)

            stmts.resolve(resolver)
            
    def validate(self, vld):
        for id in self._scope.IDs():
            id.validate(vld)
            
        for stmt in self.statements():
            stmt.validate(vld)
            
    def pac(self, printer):
        printer.output("module %s\n" % self._name, nl=True)
        
        for id in self._scope.IDs():
            id.pac(printer)
            
        for stmt in self.statements():
            stmt.pac(printer)
        
    # Visitor support.
    def visit(self, v):
        v.visit(self._scope)
    
        
    

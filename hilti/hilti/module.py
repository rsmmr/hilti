# $Id$

import llvm.core

import function
import location
import id
import node
import operand
import scope
import type

class Module(node.Node):
    """Represents a single HILTI link-time unit. A module has its
    own identifier scope defining which ~~ID objects are visibile
    inside its namespace.  
    
    name: string - The globally visible name of the module; modules
    names are treated case-insensitive. 
    
    location: ~~Location - A location to be associated with the function. 
    """
    def __init__(self, name, location=None):
        super(Module, self).__init__(location)
        self._name = name.lower()
        self._location = location
        self._scope = scope.Scope(None, self._name)
        self._imported_modules = []
        self._exported = []
        
        if name.lower() == "main":
            # Automatically export the "run" function.
            ident = id.Unknown("run", type.Unknown())
            self._exported += [ident]

    def name(self):
        """Returns the name of the module. The module's name will have been
        converted to all lower-case in accordance with the policy to treat is
        case-independent.
        
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
        """Makes the IDs of another scope available to the current one.
        
        other: ~~Module - The other module.
        """
        for i in other.scope().IDs():
        
            if i.imported():
                # Don't import IDs recursively.
                continue
            
            if i.linkage() == id.Linkage.EXPORTED:
                newid = i.clone()
                newid.setLocation(i.location())
                newid.setImported()
                newid.setNamespace(other.name())
                self.scope().add(newid)
        
        self._imported_modules += [(other, "<need to set path>")]

    def exportIdent(self, id):
        """Marks an identifier as exported. The code generator will then
        generate corresponding code making the ID referencable from external.
        
        id: ~~ID - The ID to export.
        """
        self._exported += [id]
        
    def __str__(self):
        return "module %s" % self._name

    ### Overridden from Node.
    
    def resolve(self, resolver):
        self._scope.resolve(resolver)

        new_exported = []
        for i in self._exported:
            rid = self._scope.lookup(i.name())
            new_exported += [rid if rid else i]
                
        self._exported = new_exported
        
    def validate(self, vld):
        vld.startModule(self)
        self._scope.validate(vld)

        if self.name().lower() == "main":
            run = self._scope.lookup("run")
            if not run:
                vld.error(self, "module Main must define a run() function")
                return
            
            if not isinstance(run, id.Function):
                vld.error(run, "in module Main, ID 'run' must be a function")
                return
        
        for i in self._exported:
            if isinstance(i, id.Unknown):
                vld.error(i, "export of unknown id %s" % i.name())
                continue
                
            i.validate(vld)

        vld.endModule()
    
    def canonify(self, canonifier):
        canonifier.startModule(self)

        for i in self._exported:
            i.setLinkage(id.Linkage.EXPORTED)
        
        for i in self._scope.IDs():
            i.canonify(canonifier)
            
            # id.Function does not canonify its value to avoid getting into
            # functions recursively.
            if isinstance(i, id.Function): 
                i.function().canonify(canonifier)
            
        canonifier.endModule()
            
    def codegen(self, cg):
        cg.startModule(self)
        
        for ty in self._exported:
            # Add the type information for the exported types to the module.
            if isinstance(ty, id.Type):
                cg.llvmTypeInfoPtr(ty.type())

        for i in self._scope.IDs():
            # Need to do globals first so that they are defined when functions
            # want to access them.
            if isinstance(i, id.Global):
                i.codegen(cg)

        for i in self._scope.IDs():
            # Todo: This is a bit of a hack: if this function is declared, we
            # generate it here directly. This is for libhilti to include it into
            # the library. However, we should build a nicer general mechanism
            # for static auto-generated functions.
            if i.name() == "hlt_call_bound_function":
                cg._llvmFunctionCallBoundFunction()
                continue
            
            if isinstance(i, id.Function):
                i.codegen(cg)

        cg.endModule()

    def output(self, printer):
        
        printer.startModule(self)
        printer.printComment(self)
        
        printer.output("\nmodule %s\n" % self.name(), nl=True)

        mods = [mod for (mod, path) in self._imported_modules]
        
        imported = False
        for mod in mods:
            if mod.name() == "hlt":
                # Implicitly imported.
                continue
            
            printer.output("import %s" % mod.name(), nl=True)
            imported = True

        if imported:
            printer.output("", nl=True)        
            
        self._scope.output(printer)
            
        printer.endModule()
            
    # Visitor support.
    def visit(self, v):
        v.visitPre(self)
        self.scope().visit(v)
        v.visitPost(self)
    
        
    

# $Id$

class Resolver(object):
    """Class that resolves all still unresolved identifiers in a
    module."""

    def resolve(self, mod):
        """Resolves all still unresolved identifiers in a module.
        Any errors are directly reported to the user. 
    
        mod: ~~Module - The mod. 
    
        Returns: int - The integer is the number of errors found during resolving.
        """
        self._errors = 0
        self._scope = mod.scope()
        self._functions = []
        self._modules = []
        mod.resolve(self)
        self._scope = None
        
        return self._errors
    
    def scope(self):
        """Returns the scope in which unknown identifiers should be
        resolved. Only valid while ~~resolve is running. 
        
        Returns: ~~Scope - The scope, or None if ~~resolve is not running.
        """
        return self._scope

    def startModule(self, m):
        """Indicates the start of resolving a module.
        
        m: ~~Module - The module.
        """
        self._modules += [m]
        
    def endModule(self):
        """Indicates the end of resolving a module."""
        self._module = self._modules[:-1]    
    
    def currentModule(self): 
        """Returns the current module.
        
        Returns: ~~Module - The current module.
        """
        return self._modules[-1]
    
    def startFunction(self, f):
        """Indicates that resolving starts for a new function.
        
        f: ~~Function - The function.
        """
        self._functions += [f]
        
    def endFunction(self):
        """Indicates that resolving is finished with the current function."""
        self._functions = self._functions[:-1]
    
    def currentFunction(self):
        """Returns the current function. Only valid while resolving is in
        progress.
        
        Returns: ~~Function - The current function.
        """
        return self._functions[-1]
    

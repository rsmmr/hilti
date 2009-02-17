# $Id$

from hilti.core import *

class Checker(visitor.Visitor):
    """Implements the semantic verification of an AST.  
    
    The verification process proceeds by visiting the nodes of the |ast| and
    verifying individually. Note that different from most other visitor
    classes in HILTI, the checker always executes *all* matching handlers, not
    only the one with the most specific type. See ~~Visitor for more
    information. 
    """
    
    def __init__(self):
        super(Checker, self).__init__(all=True)
        self._reset()

    def _reset(self):
        """Resets the internal state. After resetting, the object can be used
        to check another |ast|.
        """
        self._have_module = False
        self._have_others = False
        self._module = None
        self._function = None
        self._errors = 0

    def declSeen(self):
        """Indicates whether any module-wide declaration/definition has
        already been visited. This includes, e.g., function definitions, type
        declarations, etc.; yet not a +module+ statement. 
        
        Returns: bool - True if module-wide declarations have been seen.
        """
        return self._have_others

    def moduleSeen(self):
        """Indicates whether a +module+ has already been visited.
        
        Returns: bool - True if +module+ has been seen. 
        """
        return self._have_module

    def currentModule(self): 
        """Returns the current module. Only valid while visiting is in
        progress.
        
        Returns: ~~Module - The current module.
        """
        return self._module
    
    def currentFunction(self): 
        """Returns the current function. Only valid while visiting is in
        progress.
        
        Returns: ~~Function - The current function.
        """
        return self._function
    
    def checkAST(self, ast):
        """See ~~checkAST."""
        self._reset()
        self.visit(ast)
        return self._errors
        
    def error(self, obj, message):
        """Reports an error message to the user. Once this method has been
        called, no further handlers will be executed for the object currently
        being visited. However, visiting will proceed with subsequent |ast|
        nodes. 
        
        obj: object - Provides the location information to include in the
        message. *obj* must have a method *location()* returning a ~~Location
        object. Usually, *obj* is just the object currently being visited.
        """
        self._errors += 1
        util.error(message, context=obj.location(), fatal=False)        
        self.skipOthers()

checker = Checker()

        

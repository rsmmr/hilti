# $Id$

from hilti.core import *

class Checker(visitor.Visitor):
    """This :class:`~hilti.core.visitor.Visitor` implements the semantic
    verification of an AST, which is crucial to perform after any changes to
    an |ast|. The main interface to the functionality provided by this package
    is the function :func:`~hilti.checker.checkAST()`. All the other package
    content is primarily for internal use by the individial handlers.
    
    Checking proceeds by visiting the nodes of the |ast|. Each handler
    verifies the correctness of its object, reporting any errors via
    :func:`error`. 
    
    Note that different from most other visitor classes in HILTI, the Checker
    always executes all matching handlers, not only the one with the most
    specific type. See :class:`~hilti.core.visitor.Visitor`` for more
    information. 
    """
    
    def __init__(self):
        super(Checker, self).__init__(all=True)
        self._reset()

    def _reset(self):
        """Resets the internal state so that the object can be check another
        |ast|.
        """
        self._have_module = False
        self._have_others = False
        self._current_module = None
        self._current_function = None
        self._errors = 0

    def declSeen(self):
        """Returns True if any module-wide declaration/definition has already
        been visited (e.g., function definitions, type declarations, etc.)."""
        return self._have_others

    def moduleSeen(self):
        """Returns True if a +module+ statement has already been visited.""" 
        return self._have_module

    def currentModule(self): 
        """During visiting, returns the current module, or None if we haven't
        seen a +module+ statement yet."""
        return self._current_module
    
    def currentFunction(self): 
        """During visiting, returns the current function, or None if we're not
        in a function currently."""
        return self._current_function
    
    def checkAST(self, ast):
        """Checks the given *ast*."""
        self._reset()
        self.dispatch(ast)
        return self._errors
        
    def error(self, obj, message):
        """Reports the error *message* to the user. *obj* is used to provide
        location information with the message, and must have a method
        *location()* that returns a :class:`~hilti.core.location.Location`
        object. Usually, *obj* is just the object currently being visited.
        
        Once this method has been called, no further handlers will be executed
        for the object currently being visited. However, visiting will proceed
        with subsequent |ast| nodes. 
        """
        self._errors += 1
        util.error(message, context=obj.location(), fatal=False)        
        self.skipOthers()

checker = Checker()

        

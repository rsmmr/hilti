# $Id$

import sys

import function
import instruction
import node
import printer
import util

class Validator(object):
    """Implements the semantic verification of a module."""

    def validate(self, mod):
        """Verifies a module's correctness.
        
        mod: ~~Module - The module.
        """
        self._have_module = False
        self._have_others = False
        self._modules = []
        self._functions = []
        self._locations = set()
        self._errors = 0
        
        mod.validate(self)
        
        return self._errors
    
    def errors(self):
        """Returns the number of errors found so far.
        
        Returns: int - The number of errors.
        """
        return self._errors
        
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

    def startModule(self, m):
        """Indicates the start of validating a function.
        
        f: ~~Function - The function.
        """
        if self.moduleSeen():
            self.error(m, "more than one *module* statement")
        
        if self.declSeen():
            self.error(m, "*module* statement does not come first")
        
        self._have_module = True
        self._modules += [m]
        
    def endModule(self):
        """Indicates the end of validating a module."""
        self._module = self._modules[:-1]
    
    def startFunction(self, f):
        """Indicates the start of validating a function.
        
        f: ~~Function - The function.
        """
        if not self.moduleSeen():
            self.error(f, "input file must start with module declaration")
        
        self._have_others = True
        self._functions += [f]

    def endFunction(self):
        """Indicates the end of validating a module."""
        self._functions = self._functions[:-1]
    
    def currentModule(self): 
        """Returns the current module.
        
        Returns: ~~Module - The current module.
        """
        return self._modules[-1]
    
    def currentFunction(self): 
        """Returns the current function.
        
        Returns: ~~Function - The current function.
        """
        return self._functions[-1]
        
    def error(self, obj, message, indent=False):
        """Reports an error message to the user. Once this method has been
        called, no further handlers will be executed for the object currently
        being visited. However, visiting will proceed with subsequent |ast|
        nodes. 
        
        obj: object - Provides the location information to include in the
        message. *obj* must have a method *location()* returning a ~~Location
        object. Usually, *obj* is just the object currently being visited.
        
        indent: bool - If true, the text is assumed to contain more details
        about the previous error and will be printed indented and not counted
        as a new error.
        
        Note: If more than one error is reported for the same location, we
        print only the first.
        """
        
        loc = obj.location()
        
        if loc and loc.line():
            idx = str(loc)
            if idx in self._locations:
                return
            
            self._locations.add(idx)
        
        if not indent:
            self._errors += 1

        nl = False
        
        if isinstance(obj, function.Function):
            print >>sys.stderr, ">>> In function %s:" % obj.name()
            nl = True
        
        elif isinstance(obj, instruction.Instruction):
            try:
                p = printer.Printer()
                p.startModule(self.currentModule())
                p.printNode(obj, output=sys.stderr, prefix=">>> ")
                p.endModule()
            except:
                # This may fail because of the very error we are reporting.
                # Ignore.
                pass
            finally:
                p.finishLine()
            
            nl = True
        
        util.error(message, context=loc, fatal=False, indent=indent)        
        
        if nl:
            print >>sys.stderr, ""

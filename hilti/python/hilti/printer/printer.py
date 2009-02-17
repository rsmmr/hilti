# $Id$

import sys

from hilti.core import *

### Printer visitor.

class Printer(visitor.Visitor):
    """Implements the conversion of an |ast| into a HILTI program."""
    def __init__(self):
        super(Printer, self).__init__()
        self.reset()
        
    def reset(self):
        """Resets the internal state. After resetting, the object can be used
        to print another |ast|.
        """
        self._output = sys.stdout
        self._indent = 0;
        self._module = None
        self._function = None

    def printAST(self, ast, output):
        """See ~~printAST."""
        self.reset()
        self._output = file
        self.visit(ast)
        return True

    def currentModule(self):
        """Returns the current module. Only valid while printing is in
        progress.
        
        Returns: ~~Module - The current module.
        """
        return self._module
    
    def currentFunction(self):
        """Returns the current function. Only valid while printing is in
        progress.
        
        Returns: ~~Function - The current function.
        """
        return self._function
    
    def push(self):
        """Increases indentation in the output by one level.""" 
        self._indent += 1
        
    def pop(self):
        """Decreases indentation in the output by one level.""" 
        self._indent -= 1
        
    def output(self, str = "", nl=True):
        """Prints a string to the current output file.
        
        str: string - The string to print.
        nl: bool - If True, add a trailing newline. 
        """
        print >>self._output, ("    " * self._indent) + str,
        if nl:
            print >>self._output

printer = Printer()

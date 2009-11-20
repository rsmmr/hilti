# $Id$

import sys

from hilti.core import *

### Printer visitor.

class Printer(visitor.Visitor):
    """Converts an |ast| into a textal represnetation. The result is a
    parseable HILTI."""
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
        self._printing_function = False
        
    def printAST(self, ast, output, prefix=""):
        """See ~~printAST."""
        self._output = output
        self._prefix = prefix
        self.visit(ast)
        self.reset()
        return True

    def currentModule(self):
        """Returns the current module. Only valid while printing is in
        progress.
        
        Returns: ~~Module - The current module.
        """
        return self._module
    
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
        self._output.write(self._prefix)
        self._output.write("    " * self._indent + str)
        if nl:
            self._output.write("\n")

printer = Printer()

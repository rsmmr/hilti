# $Id$

import sys

def printModule(mod, output):
    """Prints the module as BinPAC++ code.
    
    mod: ~~Module - The module to print. 
    output: file - The file to write the output to. 
    """
    printer = Printer()
    printer.printNode(mod, output)

class Printer(object):
    """Converts an BinPAC++ AST into a parseable source code."""
    def __init__(self):
        super(Printer, self).__init__()
        self.reset()
        
    def reset(self):
        """Resets the internal state. After resetting, the object can be used
        to print another AST.
        """
        self._output = sys.stdout
        self._indent = 0;
        self._module = None
        self._printing_function = False
        
    def printNode(self, ast, output, prefix=""):
        """Prints out an |ast| as a HILTI program. The |ast| must be
        well-formed as verified by ~~checkAST.
    
        ast: ~~ast.Node - The root of the |ast| to print. 
        output: file - The file to write the output to. 
        prefix: string - Each output line begin with the prefix. 
        """
        self._output = output
        self._prefix = prefix
        ast.pac(self)
        self.reset()

    def push(self):
        """Increases indentation in the output by one level.""" 
        self._indent += 1
        
    def pop(self):
        """Decreases indentation in the output by one level.""" 
        self._indent -= 1
        
    def output(self, str = "", nl=False):
        """Prints a string to the current output file.
        
        str: string - The string to print.
        nl: bool - If True, add a trailing newline. 
        """
        self._output.write(self._prefix)
        self._output.write("    " * self._indent + str)
        if nl:
            self._output.write("\n")

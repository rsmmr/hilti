#! /usr/bin/env
#
# Prints a module out in re-parseable form.

import sys

from hilti.core import *

### Printer visitor.

class Printer(visitor.Visitor):
    def __init__(self):
        super(Printer, self).__init__()
        self.reset()
        
    def reset(self):
        self._output = sys.stdout
        self._indent = 0;
        self._module = None
        self._infunction = False

    def printAST(self, ast, output):
        self.reset()
        self.setOutput(output)
        self.dispatch(ast)
        return True
        
    def setOutput(self, file):
        self._output = file
        
    def push(self):
        self._indent += 1
        
    def pop(self):
        self._indent -= 1
        
    def output(self, str = "", nl=True):
        print >>self._output, ("    " * self._indent) + str,
        if nl:
            print >>self._output

printer = Printer()

# $Id$

builtin_id = id

import sys

import id
import type

class Printer(object):
    """Converts a node back into parseable source code."""
    def __init__(self):
        self._modules = []
    
    def printNode(self, node, output, prefix=""):
        """Prints a node as source code.
    
        node: ~~Node - The node to print. 
        output: file - The file to write the output to. 
        prefix: string - Each output line begin with the prefix. 
        """
        self._printing_function = False
        self._indent = 0;
        self._output = output
        self._prefix = prefix
        self._newline = True
        self._functions = []
        
        node.output(self)

    def push(self):
        """Increases indentation in the output by one level.""" 
        self._indent += 1
        
    def pop(self):
        """Decreases indentation in the output by one level.""" 
        self._indent -= 1
        
    def output(self, str = None, nl=False):
        """Prints a string to the current output file. 
        
        str: string - The string to print. 
        
        nl: bool - If True, add a trailing newline. If *str* is None
        (but *not* the empty string!), this is enabled
        automatically.
        """
        if str == None:
            str = ""
            nl = True
        
        if self._newline and str != "":
            self._output.write(self._prefix)
            self._output.write("    " * self._indent)
            
        self._output.write(str)
        
        if nl:
            self._output.write("\n")
            self._newline = True
        else:
            self._newline = False

    def finishLine(self):
        """Ends the current line in the output."""
        if not self._newline:
            self._output.write("\n")
            self._newline = True
            
    def startModule(self, m):
        """Indicates the start of printing a module.
        
        m: ~~Module - The module.
        """
        self._modules += [m]
        
    def endModule(self):
        """Indicates the end of printing a module."""
        self._modules = self._modules[:-1]
            
    def startFunction(self, f):
        """Indicates the start of printing a function.
        
        f: ~~Function - The function.
        """
        self._functions += [f]
        
    def endFunction(self):
        """Indicates the end of printing a module."""
        # Copy the transformed blocks over to the function.
        self._functions = self._functions[:-1]
        
    def currentModule(self):
        """Returns the currently printed module.
        
        Returns: ~~Module - The current module.
        """
        return self._modules[-1]
    
    def currentFunction(self):
        """Returns the currently printed function.
        
        Returns: ~~Function - The current function.
        """
        return self._functions[-1].function
            
    def printComment(self, node, prefix=None, separate=False):
        """Prints the comment associated with a node, if any.
        
        node: ~~Node - The node.
        
        prefix: string - If given, the comment will be prefixed with this
        string in parentheses.
        
        separate: bool - If true, an additional blank line is inserted before
        the comment.
        """
        
        if not node.comment():
            return
    
        if separate:
            self.output("", nl=True)
        
        prefix = "(%s) " if prefix else ""
        
        for c in node.comment():
            self.output("# %s%s" % (prefix, c), nl=True)
            
    def printType(self, ty):
        """Outputs a type. If an identifier in the current module's scope
        defines the type, the name of the ID printed instead of the type itself.
        
        ty: ~~Type - The type to print.
        """ 
        for i in self.currentModule().scope().IDs():
            if not isinstance(i, id.Type):
                continue

            if i.type() == ty:
                self.output(i.scopedName(self))
                return
                
        ty.output(self)

        

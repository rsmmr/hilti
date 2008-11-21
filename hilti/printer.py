#! /usr/bin/env
#
# Prints a module out in re-parseable form.

import sys

import block
import module
import function
import instruction
import scope
import id
import type
import visitor

### Printer visitor.

class Printer(visitor.Visitor):
    def __init__(self, output = sys.stdout):
        super(Printer, self).__init__()
        self._output = output
        self._indent = 0;
        self._module = None
        self._infunction = False
        
    def push(self):
        self._indent += 1
        
    def pop(self):
        self._indent -= 1
        
    def output(self, str = ""):
        print >>self._output, ("    " * self._indent) + str

printer = Printer()

### Overall control structures.

@printer.when(module.Module)
def _(self, m):
    self._module = m
    self.output("module %s" % m.name())
    self.output()
    
### ID definitions. 

# Generic ID handler. Shouldn't be reached as we handle all cases specifically below.
@printer.when(id.ID)
def _(self, id):
    assert False, "ID type not handled: " + str(id.type().__class__)

@printer.when(id.ID, type.FunctionType)
def _(self, f):
    # Nothing to do for function bindings, there are treated separately via the 
    # Function visitor below.
    pass
    
@printer.when(id.ID, type.StructDeclType)
def _(self, id):
    assert not self._infunction

    name = id.name()
    type = id.type().type()
    
    self.output("struct %s {" % name)
    self.push()
    
    for id in type.IDs():
        self.output("%s %s" % (id.type().name(), id.name()))
        
    self.output("}")
    self.pop()
    self.output()
    
@printer.when(id.ID, type.StorageType)
def _(self, id):
    visibility = "local" if self._infunction else "global"
    self.output("%s %s %s" % (visibility, id.type().name(), id.name()))

### Function definitions.

@printer.pre(function.Function)
def _(self, f):
    self._infunction = True
    
    type = f.type()
    
    result = type.resultType().name()
    name = f.name()
    args = ", ".join(["%s %s" % (id.type().name(), id.name())for id in type.IDs()])
    
    self.output()
    self.output("%s %s(%s) {" % (result, name, args))
    
    self.push()

@printer.post(function.Function)
def _(self, id):
    self._infunction = False
    
    self.output("}")
    self.pop()

### Block definitions.    
    
@printer.pre(block.Block)
def _(self, b):
    if b.name():
        self.pop()
        self.output("")
        
        next = " # next: %s" % b.next().name() if b.next() else ""
        self.output("%s: %s" % (b.name(), next))
        self.push()

### Instructions.
    
# Generic instruction handler that can be overidden below with specialized versions.
@printer.when(instruction.Instruction)
def _(self, i):
    
    def fmtOp(op):
        if isinstance(op, tuple) or isinstance(op, list):
            return "(%s)" % ", ".join(op)
        
        return op
    
    name = i.name()
    target = "%s = " % fmtOp(i.target().value()) if i.target() else ""
    op1 = " %s" % fmtOp(i.op1().value()) if i.op1() else ""
    op2 = " %s" % fmtOp(i.op2().value()) if i.op2() else ""
    
    self.output("%s%s%s%s" % (target, name, op1, op2))  
    

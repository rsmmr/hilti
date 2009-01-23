# $Id$
#
# Modulel-level checks as well some applying to all instructions. 

from hilti.core import *
from checker import checker

@checker.when(module.Module)
def _(self, m):
    if self._have_module:
        self.error(m, "more than one module declaration in input file")
    
    self._have_module = True

### Global ID definitions. 

@checker.when(id.ID, type.StructDeclType)
def _(self, id):
    if not self._have_module:
        self.error(id, "input file must start with module declaration")

    if self._infunction:
        self.error(id, "structs cannot be declared inside functions")
        
@checker.when(id.ID, type.StorageType)
def _(self, id):
    if not self._have_module:
        self.error(id, "input file must start with module declaration")

### Function definitions.

@checker.pre(function.Function)
def _(self, f):
    assert not self._infunction
    self._infunction = True
    
    if not self._have_module:
        self.error(f, "input file must start with module declaration")
        
@checker.post(function.Function)
def _(self, f):
    assert self._infunction
    self._infunction = False
    
    if not self._have_module:
        self.error(f, "input file must start with module declaration")
        
### Instructions.

@checker.when(instruction.Instruction)
def _(self, i):
    
    # Check that signature maches the actual operands. 
    def typeError(actual, expected, tag):
        self.error(i, "type of %s does not match signature (expected %s but is %s) " % (tag, str(expected), str(actual)))
        
    def checkOp(op, sig, tag):
        
# FIXME: Does not work for optional arguments. Doe we need these still?        
#        if op and not sig:
#           typeError(type.name(op.type()), "none", tag)
            
#        if not op and sig and not sig == None:
#           typeError("none", type.name(sig), tag)
        
        if op and sig and not op.type() == sig:
            typeError(type.name(op.type()), type.name(sig), tag)
            
    checkOp(i.op1(), i.signature().op1(), "operand 1")
    checkOp(i.op2(), i.signature().op2(), "operand 2")
    checkOp(i.op3(), i.signature().op3(), "operand 3")
    checkOp(i.target(), i.signature().target(),"target")

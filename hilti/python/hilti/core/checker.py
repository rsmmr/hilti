# $Id$
#
# Visitor to checks the validity of the visited code.
#
# Checks TODO:
#    * we do not support Jumps to labels in other functions. Check that all Jumps are local.

import block
import module
import function
import instruction
import scope
import id
import type
import visitor
import util

class Checker(visitor.Visitor):
    def __init__(self):
        super(Checker, self).__init__(all=True)
        self.reset()

    def reset(self):
        self._infunction = False
        self._have_module = False
        self._errors = 0

    def error(self, obj, str):
        self._errors += 1
        util.error(str, context=obj, fatal=False)        

checker = Checker()

def checkAST(ast):
    """Returns number of errors found."""
    checker.reset()
    checker.dispatch(ast)
    return checker._errors

### Overall control structures.

@checker.when(module.Module)
def _(self, m):
    if self._have_module:
        error("more than one module declaration in input file", context=m)
    
    self._have_module = True

### 

### Global ID definitions. 

@checker.when(id.ID, type.StructDeclType)
def _(self, id):
    if not self._have_module:
        error("input file must start with module declaration", context=id)

    if self._infunction:
        error("structs cannot be declared inside functions", context=id)
        
@checker.when(id.ID, type.StorageType)
def _(self, id):
    if not self._have_module:
        error("input file must start with module declaration", context=id)

### Function definitions.

@checker.pre(function.Function)
def _(self, f):
    assert not self._infunction
    self._infunction = True
    
    if not self._have_module:
        error("input file must start with module declaration", context=f)
        
@checker.post(function.Function)
def _(self, f):
    assert self._infunction
    self._infunction = False
    
    if not self._have_module:
        error("input file must start with module declaration", context=f)
        
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

    



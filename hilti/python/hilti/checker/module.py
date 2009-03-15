# $Id$
#
# Modulel-level checks as well those applying to all instructions. 

builtin_type = type

import types

from hilti.core import *
from checker import checker

@checker.pre(module.Module)
def _(self, m):
    if self.moduleSeen():
        self.error(m, "more than one *module* statement")
        
    if self.declSeen():
        self.error(m, "*module* statement does not come first")
        
    self._have_module = True
    self._module = m

    if m.name() == "main":
        run = m.lookupID("run")
        if not run:
            self.error(m, "module Main must define a run() function")
            
        elif not isinstance(run.type(), type.Function):
            self.error(run, "in module Main, ID 'run' must be a function")
    
    for i in m.IDs():
        if i.role() == id.Role.GLOBAL:
            
            if not isinstance(i.type(), type.ValueType) and \
               not isinstance(i.type(), type.Function) and \
               not isinstance(i.type(), type.TypeDeclType):
                self.error(i, "illegal type for global identifier")
                break
            
            if isinstance(i.type(), type.ValueType) and i.type().wildcardType():
                self.error(i, "global variable cannot have a wildcard type")
                break
    
@checker.post(module.Module)
def _(self, m):
    self._module = None
    
### Global ID definitions. 

@checker.when(id.ID, type.StructDecl)
def _(self, id):
    self._have_others = True
    
    if self.currentFunction():
        self.error(id, "structs cannot be declared inside functions")
        
@checker.when(id.ID, type.ValueType)
def _(self, id):
    self._have_others = True

### Function definitions.

@checker.pre(function.Function)
def _(self, f):
    if not self.moduleSeen():
        self.error(f, "input file must start with module declaration")
        
    self._have_others = True
    self._function = f

    for a in f.type().args():
        if isinstance(a.type(), type.Any) and f.callingConvention() != function.CallingConvention.C_HILTI:
            self.error(f, "only functions using C-HILTI calling convention can take parameters of undefined type")
            break
        
        if isinstance(a.type(), type.ValueType) and a.type().wildcardType() and f.callingConvention() != function.CallingConvention.C_HILTI:
            self.error(f, "only functions using C-HILTI calling convention can take wildcard parameters")
            break
        
    for i in f.IDs():
        if i.role() == id.Role.LOCAL:
            if not isinstance(i.type(), type.ValueType) and not isinstance(i.type(), type.Label):
                self.error(i, "local variable %s must be of storage type" % i.name())
                break
            
            if isinstance(i.type(), type.ValueType) and i.type().wildcardType():
                self.error(i, "local variable cannot have a wildcard type")
                break
        
    
@checker.post(function.Function)
def _(self, f):
    self._function = None
        
### Instructions.
@checker.when(instruction.Instruction)
def _(self, i):
    if not self.moduleSeen():
        self.error(f, "input file must start with module declaration")
        
    self._have_others = True
    
    # Check that signature maches the actual operands. 
    def typeError(actual, expected, tag):
        self.error(i, "type of %s does not match signature (expected %s, found %s) " % (tag, str(expected), str(actual)))
        
    def checkOp(op, sig, tag):
        
        if sig and op == None and not type.isOptional(sig):
            self.error(i, "%s missing" % tag)
            return

        if op and not sig:
            self.error(i, "superfluous %s" % tag)
            return

        if op and sig:
            if op.type() != sig:
                typeError(op.type(), type.fmtTypeClass(sig), tag)
                return 
        
    checkOp(i.op1(), i.signature().op1(), "operand 1")
    checkOp(i.op2(), i.signature().op2(), "operand 2")
    checkOp(i.op3(), i.signature().op3(), "operand 3")
    checkOp(i.target(), i.signature().target(),"target")

    if i.target() and not isinstance(i.target(), instruction.IDOperand):
        self.error(i, "target must be an identifier")

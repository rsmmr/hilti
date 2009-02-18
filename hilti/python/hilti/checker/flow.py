# $Id$
# 
# Checking flow-control instructions. 

from hilti.instructions import flow
from hilti.core import *
from checker import checker

def _checkFunc(checker, i, func, cc):
    if not isinstance(func, function.Function):
        checker.error(i, "not a function name")
        return False

    if func.name().startswith("__"):
        checker.error(i, "unknown function")
        return False

    if cc and func.callingConvention() != cc:
        checker.error(i, "call to function with incompatible calling convention")
    
    return True

def _checkLabel(checker, i, op):
    pass

def _checkArgs(checker, i, func, op):
    if not isinstance(op, instruction.TupleOperand):
        checker.error(i, "function arguments must be a tuple")
        return

    args = op.value()
    ids = func.type().Args()
    
    if len(args) != len(ids):
        checker.error(i, "wrong number of arguments for function")
        return
    
    for j in range(len(args)):
        if args[j].type() != ids[j].type():
            checker.error(i, "wrong type for function argument %d in call (is %s, expected %s)" % (j+1, args[j].type(), ids[j].type()))

@checker.when(flow.Jump)
def _(self, i):
    _checkLabel(self, i, i.op1())

@checker.when(flow.ReturnVoid)
def _(self, i):
    rt = self.currentFunction().type().resultType()
    if rt != type.Void:
        self.error(i, "function does not return a value")

@checker.when(flow.ReturnResult)
def _(self, i):
    rt = self.currentFunction().type().resultType()
    if rt == type.Void:
        self.error(i, "void function returns a value")
        return
        
    if rt != i.op1().type():
        self.error(i, "return type does not match function definition")
        
@checker.when(flow.IfElse)
def _(self, i):
    _checkLabel(self, i, i.op2())
    _checkLabel(self, i, i.op3())

@checker.when(flow.Call)
def _(self, i):
    func = self.currentModule().lookupIDVal(i.op1().id().name())
    if not _checkFunc(self, i, func, None):
        return

    rt = func.type().resultType()
    
    if i.target() and rt == type.Void:
        self.error(i, "function does not return a value")
        
    if not i.target() and rt != type.Void:
        self.error(i, "function returns a value")
    
    _checkArgs(self, i, func, i.op2())

@checker.when(flow.CallC)
def _(self, i):
    func = self.currentModule().lookupIDVal(i.op1().id().name())
    if not _checkFunc(self, i, func, function.CallingConvention.C):
        return

    rt = func.type().resultType()
    
    if i.target() and rt == type.Void:
        self.error(i, "C function does not return a value")
        
    if not i.target() and rt != type.Void:
        self.error(i, "C function returns a value")
    
    _checkArgs(self, i, func, i.op2())

@checker.when(flow.CallTailVoid)
def _(self, i):
    func = self.currentModule().lookupIDVal(i.op1().id().name())
    if not _checkFunc(self, i, func, function.CallingConvention.HILTI):
        return
    
    rt = func.type().resultType()
    
    if rt != type.Void:
        self.error(i, "call.tail.void calls a function returning a value")
        
    _checkArgs(self, i, func, i.op2())
    _checkLabel(self, i, i.op3())

@checker.when(flow.CallTailResult)
def _(self, i):
    func = self.currentModule().lookupIDVal(i.op1().id().name())
    if not _checkFunc(self, i, func, function.CallingConvention.HILTI):
        return
    
    rt = func.type().resultType()
    
    if rt == type.Void:
        self.error(i, "call.tail.result calls a function that does not return a value")
    
    _checkArgs(self, i, func, i.op2())
    _checkLabel(self, i, i.op3())


    

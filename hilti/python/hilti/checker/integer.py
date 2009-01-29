# $Id$
#
# Integer checks.

from hilti.instructions import integer
from hilti.core import *
from checker import checker

def _checkOp(c, op):
    # Only constant operands may have an unspecified width. 
    if op.type().width() == 0:
        if not isinstance(op, instruction.ConstOperand):
            c.error(i, "cannot use non-constant integer operand with unspecified width")

def _checkConst(val, width):
    return val < 2**width
            
def _sameWidth(c, i, target):
    if isinstance(i.op1(), instruction.ConstOperand):
        if isinstance(i.op2(), instruction.ConstOperand):
            if not target:
                # Two constants, no target. That's ok.
                return 
            
                # Two constants must match target's width.
            ok = _checkConst(i.op1().value(), i.target().type().width()) and \
                 _checkConst(i.op2().value(), i.target().type().width())
                 
            if not ok:
                c.error("constant(s) too large for target type")
                
            return
                   
        else:
            # Const must match other operand. 
            if not _checkConst(i.op1().value(), i.op2().type().width()):
                c.error("constant too large for other operand")

    else:
        if isinstance(i.op2(), instruction.ConstOperand):
            # Const must match other operand. 
            if not _checkConst(i.op2().value(), i.op1().type().width()):
                c.error("constant too large for other operand")
            return 
        
    if i.op1().type().width() != i.op2().type().width():
        c.error(i, "integer operands must have same width")

    if target and i.op1().type().width() != i.target().type().width():
        c.error(i, "integer target must have same width as operands")
        
@checker.when(integer.Add)
def _(self, i):
    _checkOp(self, i.op1())
    _checkOp(self, i.op2())
    _sameWidth(self, i, True)

@checker.when(integer.Sub)
def _(self, i):
    _checkOp(self, i.op1())
    _checkOp(self, i.op2())
    _sameWidth(self, i, True)

@checker.when(integer.Div)
def _(self, i):
    _checkOp(self, i.op1())
    _checkOp(self, i.op2())
    _sameWidth(self, i, True)

@checker.when(integer.Mul)
def _(self, i):
    _checkOp(self, i.op1())
    _checkOp(self, i.op2())
    _sameWidth(self, i, True)

@checker.when(integer.Eq)
def _(self, i):
    _checkOp(self, i.op1())
    _checkOp(self, i.op2())
    _sameWidth(self, i, False)
    
@checker.when(integer.Lt)
def _(self, i):
    _checkOp(self, i.op1())
    _checkOp(self, i.op2())
    _sameWidth(self, i, False)
    
@checker.when(integer.Trunc)
def _(self, i):
    _checkOp(self, i.op1())
    if i.op1().type().width() < i.target().type().width():
        self.error(i, "width of integer operand too small")
        
@checker.when(integer.Ext)
def _(self, i):
    _checkOp(self, i.op1())
    if i.op1().type().width() > i.target().type().width():
        self.error(i, "width of integer operand too large")

    
    
    



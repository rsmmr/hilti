# $Id$
#
#    - No terminators at other locations, split blocks where necessary.
#    - Turn call instructions into terminator call.tail.void or call.tail.result.  
#    - Turn calls to function with C calling convention into call.c instructions.
"""
The following canonifications are perfomed on flow-control instructions:

* All :class:`~hilti.instructions.Call` instructions are replaced with
  either:
    
 1. :class:`~hilti.instructions.flow.CallTailVoid` if the callee function is a
    HILTI function without any return value; or
    
 2. :class:`~hilti.instructions.flow.CallTailResult` if the callee function is
    a HILTI function without any return value; or
    
 3. :class:`~hilti.instructions.flow.CallC` if the callee function is a
    function with C calling convention.

* For all |terminator| instructions, the current
  :class:`~hilti.core.block.Block` is closed at their location and a new
  :class:`~hilti.core.block.Block` is opened.

"""

from hilti.core import *
from hilti import instructions
from canonifier import canonifier

# Terminates the current block with the given instruction and starts a new one.
def _splitBlock(canonifier, ins=None):
    if ins:
        current_block = canonifier.currentTransformedBlock()
        current_block.addInstruction(ins)
    
    new_block = block.Block(canonifier.currentFunction(), instructions = [], name=canonifier.makeUniqueLabel())
    canonifier.addTransformedBlock(new_block)
    return new_block

@canonifier.when(instructions.flow.Call)
def _(self, i):
    
    name = i.op1().value().name()
    func = self.currentModule().lookupGlobal(name)
    assert func and isinstance(func.type(), type.FunctionType)
    
    if func.linkage() == function.Function.CC_C:
        callc = instructions.flow.CallC(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        current_block = self.currentTransformedBlock()
        current_block.addInstruction(callc)
        return
    
    if i.op1().type().resultType() == type.Void:
        tc = instructions.flow.CallTailVoid(op1=i.op1(), op2=i.op2(), op3=None, location=i.location())
    else:
        tc = instructions.flow.CallTailResult(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        
    new_block = _splitBlock(self, tc)
    
    i = id.ID(new_block.name(), type.Label, location=i.location())
    label = instruction.IDOperand(i, False, location=i.location())
    tc.setOp3(label)
    new_block.setMayRemove(False)

@canonifier.when(instructions.flow.Jump)
def _(self, i):
    _splitBlock(self, i)

@canonifier.when(instructions.flow.ReturnVoid)
def _(self, i):
    _splitBlock(self, i)

@canonifier.when(instructions.flow.ReturnResult)
def _(self, i):
    _splitBlock(self, i)
    
@canonifier.when(instructions.flow.IfElse)
def _(self, i):
    _splitBlock(self, i)
    

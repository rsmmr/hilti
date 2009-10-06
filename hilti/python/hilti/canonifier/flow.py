# $Id$
#
#    - No terminators at other locations, split blocks where necessary.
#    - Turn call instructions into terminator call.tail.void or call.tail.result.  
#    - Turn calls to function with C calling convention into call.c instructions.
"""
The following canonifications are perfomed on flow-control instructions:

* All ~~Call instructions are replaced with either:
  
 1. ~~CallTailVoid if the callee function is a HILTI function without any
    return value; or
    
 2. ~~CallTailResult if the callee function is a HILTI function without any
    return value; or
    
 3. ~~CallC if the callee function is a function with C calling convention.
    
* For all |terminator| instructions, the current ~~Block is closed at their
  location and a new ~~Block is opened.

"""

from hilti.core import *
from hilti import instructions
from canonifier import canonifier

# Terminates the current block with the given instruction and starts a new one.
def _splitBlock(canonifier, ins=None, add_flow_dbg=False):
    if ins:
        current_block = canonifier.currentTransformedBlock()
        
        if add_flow_dbg and canonifier.debugMode():
            dbg = instructions.debug.message("hilti-flow", "leaving %s" % canonifier.currentFunctionName())
            current_block.addInstruction(dbg)
        
        current_block.addInstruction(ins)
    
    new_block = block.Block(canonifier.currentFunction(), instructions = [], name=canonifier.makeUniqueLabel())
    canonifier.addTransformedBlock(new_block)
    return new_block

@canonifier.when(instructions.flow.Call)
def _(self, i):
    func = self.currentModule().lookupIDVal(i.op1().value())
    assert func and isinstance(func.type(), type.Function)
    
    if func.callingConvention() in (function.CallingConvention.C, function.CallingConvention.C_HILTI):
        callc = instructions.flow.CallC(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        current_block = self.currentTransformedBlock()
        current_block.addInstruction(callc)
        return
    
    if i.op1().type().resultType() == type.Void:
        tc = instructions.flow.CallTailVoid(op1=i.op1(), op2=i.op2(), op3=None, location=i.location())
    else:
        tc = instructions.flow.CallTailResult(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        
    new_block = _splitBlock(self, tc)
    
    i = id.ID(new_block.name(), type.Label(), id.Role.LOCAL, location=i.location())
    label = instruction.IDOperand(i, location=i.location())
    tc.setOp3(label)
    new_block.setMayRemove(False)

@canonifier.when(instructions.flow.Jump)
def _(self, i):
    _splitBlock(self, i)

@canonifier.when(instructions.flow.ReturnVoid)
def _(self, i):
    _splitBlock(self, i, add_flow_dbg=True)

@canonifier.when(instructions.flow.ReturnResult)
def _(self, i):
    _splitBlock(self, i, add_flow_dbg=True)
    
@canonifier.when(instructions.flow.IfElse)
def _(self, i):
    _splitBlock(self, i)

@canonifier.when(instructions.flow.ThreadYield)
def _(self, i):
    _splitBlock(self, i, add_flow_dbg=True)

@canonifier.when(instructions.flow.Yield)
def _(self, i):
    _splitBlock(self, i)
    

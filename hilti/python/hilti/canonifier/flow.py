# $Id$
#
#    - No terminators at other locations, split blocks where necessary.
#    - Turn call instructions into terminator call.tail.void or call.tail.result.  
#    - Turn calls to function with C calling convention into call.c instructions.

from hilti.core import *
from hilti import instructions
from canonifier import canonifier

# Terminate the current block with the given instruction 
# and starts a new one.
def _splitBlock(canonifier, ins=None):
    assert canonifier._current_function
    assert canonifier._transformed_blocks
    
    if ins:
        current_block = canonifier._transformed_blocks[-1]
        current_block.addInstruction(ins)
    
    canonifier._label_counter += 1
    new_block = block.Block(canonifier._current_function, instructions = [], name="@__l%d" % (canonifier._label_counter))
    canonifier._transformed_blocks.append(new_block)

### Call    
    
@canonifier.when(instructions.flow.Call)
def _(self, i):
    
    name = i.op1().value().name()
    func = self._current_module.lookupGlobal(name)
    assert func and isinstance(func.type(), type.FunctionType)
    
    if func.linkage() == function.Function.CC_C:
        callc = instructions.flow.CallC(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        current_block = canonifier._transformed_blocks[-1]
        current_block.addInstruction(callc)
        return
    
    if i.op1().type().resultType() == type.Void:
        tc = instructions.flow.CallTailVoid(op1=i.op1(), op2=i.op2(), op3=None, location=i.location())
    else:
        tc = instructions.flow.CallTailResult(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        
    _splitBlock(self, tc)
    
    i = id.ID(self._transformed_blocks[-1].name(), type.Label, location=i.location())
    label = instruction.IDOperand(i, False, location=i.location())
    tc.setOp3(label)

### Jump    

@canonifier.when(instructions.flow.Jump)
def _(self, i):
    current_block = canonifier._transformed_blocks[-1]
    current_block.addInstruction(i)
    _splitBlock(self)

### ReturnVoid    
    
@canonifier.when(instructions.flow.ReturnVoid)
def _(self, i):
    current_block = canonifier._transformed_blocks[-1]
    current_block.addInstruction(i)
    _splitBlock(self)

### ReturnResult
    
@canonifier.when(instructions.flow.ReturnResult)
def _(self, i):
    current_block = canonifier._transformed_blocks[-1]
    current_block.addInstruction(i)
    _splitBlock(self)
    

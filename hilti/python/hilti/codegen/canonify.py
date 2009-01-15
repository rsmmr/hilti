# $Id$
#
# Visitor to canonify a module's code before CPS and code generation can take place.
#
# We apply the following transformations:
#    - Each blocks ends with a terminator. 
#    - No terminators at other locations, split blocks where necessary.
#    - Turn call instructions into terminator call.tail.void or call.tail.result.  
#    - Each block get name unique within its function. 

import sys

import llvm
import llvm.core 

from hilti.core import *
from hilti import ins

### Helpers

# Returns true if instruction terminates a block in canonified form.
def _isTerminator(i):
    return isinstance(i, ins.flow.Jump) or \
           isinstance(i, ins.flow.IfElse) or \
           isinstance(i, ins.flow.CallTailVoid) or \
           isinstance(i, ins.flow.CallTailResult) or \
           isinstance(i, ins.flow.ReturnVoid) or \
           isinstance(i, ins.flow.ReturnResult)

# If the last instruction is not a terminator, add it. 
def unifyBlock(block):
    instr = block.instructions()
    if not len(instr) or not _isTerminator(instr[-1]):
        if block.next():
            newins = ins.flow.Jump(instruction.IDOperand(id.ID(block.next().name(), type.Label), True))
        else:
            newins = ins.flow.ReturnVoid(None)
        block.addInstruction(newins)

### Canonify visitor.

class Canonify(visitor.Visitor):
    def __init__(self):
        super(Canonify, self).__init__()
        self.reset()
        
    def reset(self):
        self._transformed = None
        self._current_function = None
        self._label_counter = 0
        
canonify = Canonify()

def canonifyAST(ast):
    """Canonifies *ast* in place"""
    canonify.reset()
    canonify.dispatch(ast)
    return True

### Function

@canonify.pre(function.Function)
def _(self, f):
    self._current_function = f
    self._label_counter = 0
    self._transformed_blocks = []
    
    # Chain blocks together where not done yet.
    prev = None
    for b in f.blocks():
        if prev and not prev.next():
            prev.setNext(b)

@canonify.post(function.Function)
def _(self, f):
    assert self._transformed_blocks
    
    f.clearBlocks()
    for b in self._transformed_blocks:
        if b.instructions():
            f.addBlock(b)

    for b in f.blocks():
        unifyBlock(b)
        
### Block

@canonify.pre(block.Block)
def _(self, b):
    
    # If first block doesn't have a name, call it like the function.
    name = b.name()
    if not self._transformed_blocks and not name:
        name = "@__%s" % canonify._current_function.name()
        
    # While we proceed, we copy each instruction over to a new block, 
    # potentially after transforming it first.
    self._transformed_blocks += [block.Block(instructions=[], name=name, location=b.location())]

### Instructions.

@canonify.when(instruction.Instruction)
def _(self, i):
    assert self._transformed_blocks
    
    # Default just leaves instruction untouched.
    current_block = self._transformed_blocks[-1]
    current_block.addInstruction(i)

# Terminate the current block with the given instruction 
# and starts a new one.
def _splitBlock(canonify, ins=None):
    assert canonify._current_function
    assert canonify._transformed_blocks
    
    if ins:
        current_block = canonify._transformed_blocks[-1]
        current_block.addInstruction(ins)
    
    canonify._label_counter += 1
    new_block = block.Block(instructions = [], name="@__l%d" % (canonify._label_counter))
    canonify._transformed_blocks.append(new_block)
    
@canonify.when(ins.flow.Call)
def _(self, i):
    if not i.target():
        tc = ins.flow.CallTailVoid(op1=i.op1(), op2=i.op2(), op3=None, location=i.location())
    else:
        tc = ins.flow.CallTailResult(op1=i.op1(), op2=i.op2(), op3=None, target=i.target(), location=i.location())
        
    _splitBlock(self, tc)
    label = constant.Constant(self._transformed_blocks[-1].name(), type.Label, location=i.location())
    tc.setOp3(instruction.ConstOperand(label, True))
    
@canonify.when(ins.flow.Jump)
def _(self, i):
    current_block = canonify._transformed_blocks[-1]
    current_block.addInstruction(i)
    _splitBlock(self)

@canonify.when(ins.flow.ReturnVoid)
def _(self, i):
    current_block = canonify._transformed_blocks[-1]
    current_block.addInstruction(i)
    _splitBlock(self)

@canonify.when(ins.flow.ReturnResult)
def _(self, i):
    current_block = canonify._transformed_blocks[-1]
    current_block.addInstruction(i)
    _splitBlock(self)


    
    
    

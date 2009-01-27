# $Id$
#
# Module-level structures as well as generic instruction canonification.
#
# We apply the following transformations:
#    - Each blocks ends with a terminator. 
#    - Each block get name unique within its function. 

from hilti.core import *
from hilti import instructions
from canonifier import canonifier

### Helpers

# Returns true if instruction terminates a block in canonified form.
def _isTerminator(i):
    return isinstance(i, instructions.flow.Jump) or \
           isinstance(i, instructions.flow.IfElse) or \
           isinstance(i, instructions.flow.CallTailVoid) or \
           isinstance(i, instructions.flow.CallTailResult) or \
           isinstance(i, instructions.flow.ReturnVoid) or \
           isinstance(i, instructions.flow.ReturnResult) or \
           isinstance(i, instructions.flow.IfElse)

# If the last instruction is not a terminator, add it. 
def _unifyBlock(block):
    instr = block.instructions()
    if not len(instr) or not _isTerminator(instr[-1]):
        if block.next():
            newins = instructions.flow.Jump(instruction.IDOperand(id.ID(block.next().name(), type.Label), True))
        else:
            newins = instructions.flow.ReturnVoid(None)
        block.addInstruction(newins)

#### Module

@canonifier.pre(module.Module)
def _(self, m):
    self._current_module = m
    
@canonifier.post(module.Module)
def _(self, m):
    self._current_module = None

### Function

@canonifier.pre(function.Function)
def _(self, f):
    if f.isImported():
        return
    
    self._current_function = f
    self._label_counter = 0
    self._transformed_blocks = []
    
    # Chain blocks together where not done yet.
    prev = None
    for b in f.blocks():
        if prev and not prev.next():
            prev.setNext(b)

@canonifier.post(function.Function)
def _(self, f):
    if f.isImported():
        return
    
    assert self._transformed_blocks
    
    f.clearBlocks()
    for b in self._transformed_blocks:
        if b.instructions():
            f.addBlock(b)

    for b in f.blocks():
        _unifyBlock(b)

### Block

@canonifier.pre(block.Block)
def _(self, b):
    
    # If first block doesn't have a name, call it like the function.
    name = b.name()
    if not self._transformed_blocks and not name:
        name = "@__%s" % canonifier._current_function.name()
        
    # While we proceed, we copy each instruction over to a new block, 
    # potentially after transforming it first.
    self._transformed_blocks += [block.Block(self._current_function, instructions=[], name=name, location=b.location())]

### Generic instructions.

@canonifier.when(instruction.Instruction)
def _(self, i):
    assert self._transformed_blocks
    
    # Default just leaves instruction untouched.
    current_block = self._transformed_blocks[-1]
    current_block.addInstruction(i)


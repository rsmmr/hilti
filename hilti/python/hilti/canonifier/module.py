# $Id$
#
# Module-level structures as well as generic instruction canonification.
"""
The following :class:`~hilti.core.block.Block` canonifications are performed:

* Blocks that don't have successor yet are linked to the block following them
  with the functions list of :meth:`~hilti.core.function.Function.blocks`, if
  any.
  
* Each block ends with a |terminator|. If none is already in place, we either
  add a :class:`~hilti.instructions.flow.Jump` pointing the succeeding block (as indicated
  by :meth:`~hilti.core.block.Block.next`, or a :class:`~hilti.instructions.flow.ReturnVoid`
  if there is no successor. In the later case, the function must of return
  type *void*. 
  
* Each block is assigned a *name* which is unique at least witin the function
  the block is part of.
  
* Empty blocks are removed if :meth:`~hilti.core.block.mayRemove` indicates
  that it is permissible to do so.  
"""

from hilti.core import *
from hilti import instructions
from canonifier import canonifier

### Helpers

# If the last instruction is not a terminator, add it. 
def _unifyBlock(block):
    instr = block.instructions()
    loc = block.function().location() if block.function() else None
    
    add_terminator = False
    
    if not len(instr) and not block.mayRemove():
        add_terminator = True
        
    if len(instr) and not instr[-1].signature().terminator():
        add_terminator = True
        
    if add_terminator:
        if block.next():
            newins = instructions.flow.Jump(instruction.IDOperand(id.ID(block.next().name(), type.Label, id.Role.LOCAL, location=loc)), location=loc)
        else:
            newins = instructions.flow.ReturnVoid(None, location=loc)
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
    if not f.blocks():
        return
    
    self._current_function = f
    self._label_counter = 0
    self._transformed = []
    
    # Chain blocks together where not done yet.
    prev = None
    for b in f.blocks():
        if prev and not prev.next():
            prev.setNext(b)

@canonifier.post(function.Function)
def _(self, f):
    if not f.blocks():
        return
    
    self._current_function = None

    # Copy the transformed blocks over to the function.
    f.clearBlocks()
    for b in self.transformedBlocks():
        _unifyBlock(b)
        if b.instructions():
            f.addBlock(b)

### Block

@canonifier.pre(block.Block)
def _(self, b):
    
    # If first block doesn't have a name, call it like the function.
    name = b.name()
    if not self.transformedBlocks() and not name:
        name = "@__%s" % canonifier._current_function.name()
        
    # While we proceed, we copy each instruction over to a new block, 
    # potentially after transforming it first.
    new_block = block.Block(self.currentFunction(), instructions=[], name=name, location=b.location())
    new_block.setMayRemove(b.mayRemove())
    self.addTransformedBlock(new_block)

### Generic instructions.

@canonifier.when(instruction.Instruction)
def _(self, i):
    # Default just leaves instruction untouched.
    current_block = self.currentTransformedBlock()
    assert current_block
    current_block.addInstruction(i)


# $Id$
#
# Module-level structures as well as generic instruction canonification.
"""
The following :class:`~hilti.core.block.Module` canonifications are performed:

* If we find a ``Main::run`` function, we implicitly export it.

The following :class:`~hilti.core.block.Block` canonifications are performed:

* Blocks that don't have a successor are linked to the block following them
  within their function's list of ~~Block objects, if any.
  
* All blocks will end with a |terminator|. If none is already in place, we
  either add a ~~Jump pointing the succeeding block (as indicated by
  :meth:`~hilti.core.block.Block.next`, or a ~~ReturnVoid if there is no
  successor. In the latter case, the function must be of return type ~~Void. 
  
* Each so far name-less block gets a name which is unique at least within the
  function the block is part of.
  
* Empty blocks are removed if ~~mayRemove indicates that it is permissible to
  do so.  
"""

from hilti.core import *
from hilti import instructions
from canonifier import canonifier

import sys

### Helpers

# If the last instruction is not a terminator, add it. 
def _unifyBlock(block):
    instr = block.instructions()
    loc = block.function().location() if block.function() else None
    
    add_terminator = False
    
    if not len(instr):
        add_terminator = True
        
    if len(instr) and not instr[-1].signature().terminator():
        add_terminator = True
        
    if add_terminator:
        if block.next():
            newins = instructions.flow.Jump(instruction.IDOperand(id.ID(block.next().name(), type.Label(), id.Role.LOCAL, location=loc)), location=loc)
        else:
            if canonifier.debugMode():
                dbg = instructions.debug.message("hilti-flow", "leaving %s" % canonifier.currentFunctionName())
                block.addInstruction(dbg)

            newins = instructions.flow.ReturnVoid(None, location=loc)

        block.addInstruction(newins)

#### Module

@canonifier.pre(module.Module)
def _(self, m):
    self._module = m
    
@canonifier.post(module.Module)
def _(self, m):
    self._module = None

### Function

@canonifier.pre(function.Function)
def _(self, f):
    if not f.blocks():
        return

    self._function = f
    self._label_counter = 0
    self._transformed = []

    if self.debugMode():
        args = [instruction.IDOperand(id) for id in canonifier.currentFunction().type().args()]
        fmt = ["%s"] * len(args)
        dbg = instructions.debug.message("hilti-flow", "entering %s(%s)" % (canonifier.currentFunctionName(), ", ".join(fmt)), args)
        f.blocks()[0].addInstructionAtFront(dbg)
    
    # Chain blocks together where not done yet.
    prev = None
    for b in f.blocks():
        if prev and not prev.next():
            prev.setNext(b)

@canonifier.post(function.Function)
def _(self, f):
    if not f.blocks():
        return
    
    # Copy the transformed blocks over to the function.
    f.clearBlocks()
    for b in self.transformedBlocks():
        if b.instructions() or not b.mayRemove():
            f.addBlock(b)

    for b in f.blocks():
        _unifyBlock(b)

    self._function = None
            
### Block

@canonifier.pre(block.Block)
def _(self, b):
    
    # If first block doesn't have a name, call it like the function.
    name = b.name()
    if not self.transformedBlocks() and not name:
        #        name = "@__%s" % canonifier._function.name()
        name = "@entry"
        
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

# Add debugging information.    
    
@canonifier.pre(instruction.Instruction, always=True)
def _(self, i):
    if self.debugMode() and not isinstance(i, instructions.debug.Msg):
        def replace(op):
            if isinstance(op.type(), type.Function):
                return instruction.ConstOperand(constant.Constant(op.value().name(), type.String()))
            
            if isinstance(op.type(), type.Label):
                return instruction.ConstOperand(constant.Constant(op.value().name(), type.String()))

            if not isinstance(op.type(), type.HiltiType):
                return instruction.ConstOperand(constant.Constant("<%s>" % op.type().name(), type.String()))
            
            return op
        
        args = [replace(op) for op in (i.op1(), i.op2(), i.op3()) if op]
        fmt = ["%s"] * len(args)
        dbg = instructions.debug.message("hilti-trace", "%s %s %s" % (i.location(), i.name(), " ".join(fmt)), args)
        current_block = self.currentTransformedBlock()
        current_block.addInstruction(dbg)


@canonifier.post(instruction.Instruction, always=True)
def _(self, i):
    if self.debugMode() and i.target() and not isinstance(i, instructions.debug.Msg):
        dbg = instructions.debug.message("hilti-trace", "%s -> %%s" % i.location(), [i.target()])
        current_block = self.currentTransformedBlock()
        current_block.addInstruction(dbg)

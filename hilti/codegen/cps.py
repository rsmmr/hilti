# $Id$
#
# Visitor to canonify a module's code before code generation can take place.
#
# We apply the following transformations:
#
#    - All Call instructions are transformed into TailCalls.
#    - Blocks are split after each Jump/Return instruction.
#    - Each block is ended with either a TailCall, Jump or Return instruction.
#
# FIXME: Rename from CPS to something more generic.

import llvm
import llvm.core 

import block
import module
import function
import instruction
import scope
import id
import type
import util
import constant
import visitor

import ins.flow

### CPS visitor.

class CPSTransform(visitor.Visitor):
    def __init__(self):
        super(CPSTransform, self).__init__()
        self._transformed = None
        self._current_function = None
        self._label_counter = 0
        
cps = CPSTransform()

### Function

@cps.pre(function.Function)
def _(self, f):
    self._current_function = f
    self._label_counter = 0
    self._transformed_blocks = []
    
    # Chain blocks together where not done yet.
    prev = None
    for b in f.blocks():
        if prev and not prev.next():
            prev.setNext(b)

@cps.post(function.Function)
def _(self, f):
    assert self._transformed_blocks
    
    f.clearBlocks()
    for b in self._transformed_blocks:
        f.addBlock(b)
    
### Block

@cps.pre(block.Block)
def _(self, b):
    # While we proceed, we copy each instruction over to a new block, 
    # potentially after transforming it first.
    self._transformed_blocks += [block.Block(instructions = [], name = b.name(), location = b.location())]

@cps.post(block.Block)
def _(self, b):
    # If the last instruction is not a jump or return, add it. 
    current_block = self._transformed_blocks[-1]
    instr = current_block.instructions()
    if not len(instr) or (not isinstance(instr[-1], ins.flow.TailCall) and not isinstance(instr[-1], ins.flow.Jump) and not isinstance(instr[-1], ins.flow.Return)):
        if current_block.next():
            newins = ins.flow.Jump(instruction.IDOperand(id.ID(current_block.next(), type.Label)))
        else:
            assert self._current_function.type().resultType() == type.Void 
            newins = ins.flow.Return(instruction.ConstOperand(constant.Void))
        current_block.addInstruction(newins)
    
### Instructions.

@cps.when(instruction.Instruction)
def _(self, i):
    assert self._transformed_blocks
    
    # Default just leaves instruction untouched.
    current_block = self._transformed_blocks[-1]
    current_block.addInstruction(i)

# Terminate the current block with the given instruction 
# and starts a new one.

def _splitBlock(cps, ins=None):
    assert cps._current_function
    assert cps._transformed_blocks
    
    if ins:
        current_block = cps._transformed_blocks[-1]
        current_block.addInstruction(ins)
    
    cps._label_counter += 1
    new_block = block.Block(instructions = [], name="__%s%d" % (cps._current_function.name(), cps._label_counter))
    cps._transformed_blocks.append(new_block)
    
@cps.when(ins.flow.Call)
def _(self, i):
    tc = ins.flow.TailCall(i.op1(), i.op2(), i.target(), i.location())
    _splitBlock(self, tc)
    
@cps.when(ins.flow.Jump)
def _(self, i):
    _splitBlock(self)

@cps.when(ins.flow.Return)
def _(self, i):
    _splitBlock(self)
    
#@cps.when(ins.flow.JumpIf)
#def _(self, i):
#    pass
    
    
    
    
    
    

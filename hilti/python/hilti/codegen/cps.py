# $Id$
#
# Visitor to perform tranformation into CPS form. 
# "Canonify" pass must have been applied already. 

import sys

import llvm
import llvm.core 

from hilti.core import *
import hilti.ins.flow

### CPS visitor.

class CPSTransform(visitor.Visitor):
    def __init__(self):
        super(CPSTransform, self).__init__()
        self._transformed = None
        self._current_function = None
        self._label_counter = 0

cps = CPSTransform()
      
DOESN't WORK - the three setps must be done during the LLVM transformation
as we can't really express the sematnics in HILTI.

# We split the process into a number of passes:
#
#   (1) Turn all blocks into functions.
#       (This might not be the nicest transformation but it's the easiest
#       and shouldn't have any performance penalty after optimization.)
#   (2) Turn all terminators into Call.Tails. 
#   (3) Turn all Call.Tails returning a value into call-assign sequences.

def _convertBlocksToFunctions(func): 

    if not func.blocks():
        return
    
    oldblocks = func.blocks()
    
    # First block stays with old function.
    first = oldblocks[0]
    func.clearBlocks()
    func.addBlock(first)
    first.setName(None)

    newfuncs = []
    
    # Create new functions.
    for b in oldblocks[1:]:
        name = b.name()
        if name.startswith("__"):
            name = name[2:]
        
        name = "__%s_%s" % (func.name(), name)
        blockfunc = function.Function(name=name, type=func.type(), parentfunc=func, location=b.location())
        b.setName(None)
        blockfunc.addBlock(b)
        newfuncs += [blockfunc]
        
    return newfuncs

@cps.when(module.Module)
def _(self, m):
    self._module = m

@cps.pre(function.Function)
def _(self, f):
    funcs = _convertBlocksToFunctions(f)
    for f in funcs:
        self._module.addFunction(f)
        

# $Id$
#
# Code generator for overlay instructions.
#
# Overlays keep two kinds of state:
#
#   - The starting position corresponding to offset 0. This is set by the
#     "attach" instruction.
#
#   - For each field that is a dependency for determining another field's
#     starting offset, we cache its end position once we have calculated it
#     once. 


import llvm.core

from hilti.core import *
from hilti import instructions
from codegen import codegen

_doc_c_conversion = _doc_c_conversion = """An ``overlay`` is mapped to a ``__hlt_overlay``."""

def _iterType():
    return codegen.llvmTypeConvert(type.IteratorBytes())

def _arraySize(overlay):
    return 1 + overlay.numDependencies()

def _llvmOverlayType(overlay):
    return llvm.core.Type.array(_iterType(), _arraySize(overlay))

def _unsetIter():
    return codegen.llvmConstDefaultValue(type.IteratorBytes())

@codegen.typeInfo(type.Overlay)
def _(t):
    typeinfo = codegen.TypeInfo(t)
    typeinfo.c_prototype = "__hlt_overlay";
    return typeinfo

@codegen.llvmDefaultValue(type.Overlay)
def _(t):
    return llvm.core.Constant.array(_iterType(), [_unsetIter()] * _arraySize(t))

@codegen.llvmType(type.Overlay)
def _(t, refine_to):
    return _llvmOverlayType(t)

import sys

@codegen.when(instructions.overlay.Attach)
def _(self, i):
    overlayt = i.op1().type()
    
    # Save the starting offset.
    ov = self.llvmOp(i.op1())
    iter = self.llvmOp(i.op2())
    ov = codegen.llvmInsertValue(ov, 0, iter)
    
    # Clear the cache
    for j in range(1, _arraySize(overlayt) + 1):
        ov = codegen.llvmInsertValue(ov, j, _unsetIter())
        
    # Need to rewrite back into the original value.
    codegen.llvmStoreInTarget(i.op1(), ov)

def _isNull(ov, idx, block_yes, block_no):    
    # FIXME: I guess we should do this check somewhere in the bytes code as
    # technically we can't know here what the internal representation of an
    # iterator is ...
    val = codegen.llvmExtractValue(ov, idx)
    chunk = codegen.llvmExtractValue(ov, 0)
    cmp = self.builder().icmp(llvm.core.IPRED_EQ, chunk, llvm.core.Constant.null(codegen.llvmGenericPointer()))
    self.builder().branch(block_yes, block_no)
    
@codegen.when(instructions.overlay.Get)
def _(self, i):
    # FIXME: No check for being attached yet. Need to raise an
    # OverlayNotAttached exception if not. 
    
    # FIXME: We don't check currently if we already have cached the end of a
    # depenency; if so, we should avoid redoing it.

    overlayt = i.op1().type()
    ov = self.llvmOp(i.op1())
    offset0 = codegen.llvmExtractValue(ov, 0)
    
    def _emitOne(ov, field):
        offset = field.offset()
        
        if offset == 0:
            # Starts right at the beginning.
            begin = offset0
            
        elif offset > 0:
            # Static offset. We can calculate the starting position directly.
            begin = self.llvmGenerateCCallByName("__Hlt::bytes_pos_incr_by", 
            [offset0, codegen.llvmConstInt(offset, 32)], [type.IteratorBytes(), type.Integer(32)], llvm_args=True)
            
        else:
            # Offset is not staticb ut our immediate dependency must have
            # already been calculated at this point so we take it's end
            # position out of the cache.
            deps = field.dependants()
            assert deps
            dep = overlayt.field(deps[-1]).depIndex()
            assert dep > 0
            begin = codegen.llvmExtractValue(ov, dep)
                    
        (val, end) = codegen.llvmUnpack(field.type, begin, None, field.fmt)
        
        if field.depIndex() >= 0:
            ov = codegen.llvmInsertValue(ov, field.depIndex(), end)
        
        return (ov, val)

    field = overlayt.field(i.op2().value())
    
    for dep in field.dependants():
        (ov, val) = _emitOne(ov, overlayt.field(dep))

    (ov, val) = _emitOne(ov, field)
    self.llvmStoreInTarget(i.target(), val)
            
        
    

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

import sys

_doc_c_conversion = _doc_c_conversion = """An ``overlay`` is mapped to a ``hlt_overlay``."""

def _iterType():
    return codegen.llvmTypeConvert(type.IteratorBytes(type.Bytes()))

def _arraySize(overlay):
    return 1 + overlay.numDependencies()

def _llvmOverlayType(overlay):
    return llvm.core.Type.array(_iterType(), _arraySize(overlay))

def _unsetIter():
    return codegen.llvmConstDefaultValue(type.IteratorBytes(type.Bytes()))

@codegen.typeInfo(type.Overlay)
def _(t):
    typeinfo = codegen.TypeInfo(t)
    typeinfo.c_prototype = "hlt_overlay";
    return typeinfo

@codegen.llvmDefaultValue(type.Overlay)
def _(t):
    return llvm.core.Constant.array(_iterType(), [_unsetIter()] * _arraySize(t))

@codegen.llvmType(type.Overlay)
def _(t, refine_to):
    return _llvmOverlayType(t)

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
    chunk = codegen.llvmExtractValue(val, 0)
    cmp = codegen.builder().icmp(llvm.core.IPRED_EQ, chunk, llvm.core.Constant.null(codegen.llvmTypeGenericPointer()))
    codegen.builder().cbranch(cmp, block_yes, block_no)
    
@codegen.when(instructions.overlay.Get)
def _(self, i):
    overlayt = i.op1().type()
    ov = self.llvmOp(i.op1())
    offset0 = codegen.llvmExtractValue(ov, 0)
    
    # Generate the unpacking code for one field, assuming all dependencies are
    # already resolved.
    def _emitOne(ov, field):
        offset = field.offset()
        
        if offset == 0:
            # Starts right at the beginning.
            begin = offset0
            
        elif offset > 0:
            # Static offset. We can calculate the starting position directly.
            begin = self.llvmGenerateCCallByName("hlt::bytes_pos_incr_by", 
            [offset0, codegen.llvmConstInt(offset, 32)], [type.IteratorBytes(type.Bytes()), type.Integer(32)], llvm_args=True)
            
        else:
            # Offset is not static but our immediate dependency must have
            # already been calculated at this point so we take it's end
            # position out of the cache.
            deps = field.dependants()
            assert deps
            dep = overlayt.field(deps[-1]).depIndex()
            assert dep > 0
            begin = codegen.llvmExtractValue(ov, dep)
                    
        arg = self.llvmOp(field.arg()) if field.arg() else None
        (val, end) = codegen.llvmUnpack(field.type(), begin, None, field.fmt(), arg)
        
        if field.depIndex() >= 0:
            ov = codegen.llvmInsertValue(ov, field.depIndex(), end)
        
        return (ov, val)

    # Generate code for all depedencies.
    def _makeDep(ov, field):

        if not field.dependants():
            return ov

        # Need a tmp or a PHI node. LLVM's mem2reg should optimize the tmp
        # away so this is easier. 
        # FIXME: Check that mem2reg does indeed.
        new_ov = codegen.llvmAlloca(ov.type)         
        dep = overlayt.field(field.dependants()[-1])
        block_cont = self.llvmNewBlock("have-%s" % dep.name)
        block_cached = self.llvmNewBlock("cached-%s" % dep.name)
        block_notcached = self.llvmNewBlock("not-cached-%s" % dep.name)

        _isNull(ov, dep.depIndex(), block_notcached, block_cont)
        
        self.pushBuilder(block_notcached)
        ov = _makeDep(ov, dep)
        self.builder().store(ov, new_ov)
        self.builder().branch(block_cont)
        self.popBuilder()
        
        self.pushBuilder(block_cached)
        self.builder().store(ov, new_ov)
        self.builder().branch(block_cont)
        self.popBuilder()
        
        self.pushBuilder(block_cont)
        ov = self.builder().load(new_ov)
        (ov, val) = _emitOne(ov, dep)
        return ov
    
    # Make sure we're attached.
    block_attached = self.llvmNewBlock("attached")
    block_notattached = self.llvmNewBlock("not-attached")
    
    self.pushBuilder(block_notattached)
    self.llvmRaiseExceptionByName("hlt_exception_overlay_not_attached", i.location())
    self.popBuilder()

    _isNull(ov, 0, block_notattached, block_attached)
    
    self.pushBuilder(block_attached) # Leave on stack.
    
    field = overlayt.field(i.op2().value())
    ov = _makeDep(ov, field)
    (ov, val) = _emitOne(ov, field)
    self.llvmStoreInTarget(i.target(), val)

        
    

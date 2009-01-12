#! $Id$
#
# A set of helper functions for LLVM code-generation. 

import sys

import llvm
import llvm.core

import util
import type
import instruction

# Converts a StorageType into the corresponding type for LLVM variable declarations.
def typeToLLVM(t):
    
    mapping = {
        type.String: llvm.core.Type.array(llvm.core.Type.int(8), 0),
        type.Bool:   llvm.core.Type.int(1), 
    }

    if isinstance(t, type.IntegerType):
        return llvm.core.Type.int(t.width())
    
    try:
        return mapping[t]
    except KeyError:
        util.internal_error("codegen: cannot translate type %s" % t.name())


basic_frame_th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())
continuation_th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())

continuation = llvm.core.Type.struct((
    llvm.core.Type.label(),                # Label of entry point
    llvm.core.Type.pointer(basic_frame_th.type) # Frame pointer
    ))

continuationIndices = {
    "__label": 0,
    "__frame": 1,
    }
    
basic_frame = llvm.core.Type.struct((
    llvm.core.Type.pointer(continuation_th.type), # cont_normal
    llvm.core.Type.pointer(continuation_th.type), # cont_exception
    llvm.core.Type.int(16),                  # current_exception
    ))

basic_frameIndices = {
    "__cont_normal": 0,
    "__cont_exception": 1,
    "__current_exception": 2,
    }

continuation_th.type.refine(continuation)    
basic_frame_th.type.refine(basic_frame)    
    
# Returns the LLVM struct for a Continuation.
def structContinuation():
    return continuation

# Returns the index of the given field in the Continuation struct. 
def fieldIndexContinuation(field):
    try:
        return continuationIndices[field]
    except KeyError:
        util.internal_error("unknown continuation field %s" % field)

# Returns the LLVM struct for a BasicFrame.
def structBasicFrame():
    return basic_frame

# Returns the index of the given field in the basic frame struct. 
def fieldIndexBasicFrame(field):
    try:
        return basic_frameIndices[field]
    except KeyError:
        uti.internal_error("unknown continuation field %s" % field)

# Returns the LLVM struct for the function's frame.
def structFunctionFrame(f):
    fields = [("__bf", structBasicFrame())]
    fields += [(id.name(), typeToLLVM(id.type())) for id in f.scope().IDs().values()]
    
    # Build map of indices.
    f.frameindex = {}
    for i in range(0, len(fields)):
        f.frameindex[fields[i][0]] = i

    frame = [f[1] for f in fields]
    return llvm.core.Type.struct(frame)

# Returns the index of the given field in the function's frame struct. 
# Note that structFunctionFrame() must have been called at least once for this function first.
def fieldIndexFunctionFrame(func, field):
    return func.frameindex[field]

# Returns the LLVM type *name* of the function's frame.
def nameFunctionFrame(f):
    return "__frame_%s" % f.name()

# Turns an operand into LLVM speak.
def opToLLVM(cg, op, tag):
    if isinstance(op, instruction.ConstOperand):
        return constOpToLLVM(op)

    if isinstance(op, instruction.IDOperand):
        if op.isLocal():
            zero = llvm.core.Constant.int(llvm.core.Type.int(), 0)
            
            index = fieldIndexFunctionFrame(cg._function, op.value())
            index = llvm.core.Constant.int(llvm.core.Type.int(), index)
            
            # The Python interface (as well as LLVM's C interface, which is used
            # by the Python interface) does not have builder.extract_value() method,
            # so we simulate it with a gep/load combination.
            addr = cg._llvm.builder.gep(cg._llvm.frameptr, [zero, index], tag)
            return cg._llvm.builder.load(addr, tag)
            
    util.internal_error("opToLLVM: unknown op class: %s" % op)

# Stores value in target.
def targetToLLVM(cg, target, val, tag):
    if isinstance(target, instruction.IDOperand):
        if target.isLocal():
            zero = llvm.core.Constant.int(llvm.core.Type.int(), 0)
            
            index = fieldIndexFunctionFrame(cg._function, target.value())
            index = llvm.core.Constant.int(llvm.core.Type.int(), index)
            
            # The Python interface (as well as LLVM's C interface, which is used
            # by the Python interface) does not have builder.extract_value() method,
            # so we simulate it with a gep/load combination.
            addr = cg._llvm.builder.gep(cg._llvm.frameptr, [zero, index], tag)
            cg._llvm.builder.store(val, addr)
            return 
            
    util.internal_error("targetToLLVM: unknown target class: %s" % target)
    
def constOpToLLVM(op):
    if isinstance(op.type(), type.IntegerType):
        return llvm.core.Constant.int(typeToLLVM(op.type()), op.value())
        
    if isinstance(op.type(), type.StringType):
        return llvm.core.Constant.string(op.value())
    
    util.internal_error("constOpToLLVM: illegal type %s" % op.type())


# Returns the name for the function the block name is converted into.
# Function is the function the block is part of.
def blockFunctionName(name, function):
    if name == function.blocks()[0].name():
        return function.name()

    if name.startswith("__"):
        name = name[2:]
    
    return "__%s_%s" % (function.name(), name)


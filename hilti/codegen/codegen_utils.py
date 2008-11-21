#! $Id$
#
# A set of helper functions for LLVM code-generation. 

import llvm
import llvm.core

import type

# Converts a StorageType into the corresponding type for LLVM variable declarations.
def typeToLLVM(t):
    
    mapping = {
        type.Int16:  llvm.core.Type.int(16), 
        type.Int32:  llvm.core.Type.int(32), 
        type.String: llvm.core.Type.array(llvm.core.Type.int(8), 0),
        type.Bool:   llvm.core.Type.int(1), 
    }

    try:
        return mapping[t]
    except IndexError:
        util.internal_error("codegen: cannot translate type %s" % t.name())


basic_frame_th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())
continuation_th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())

continuation = llvm.core.Type.struct((
    llvm.core.Type.label(),                # Label of entry point
    llvm.core.Type.pointer(basic_frame_th.type) # Frame pointer
    ))

basic_frame = llvm.core.Type.struct((
    llvm.core.Type.pointer(continuation_th.type), # cont_normal
    llvm.core.Type.pointer(continuation_th.type), # cont_exception
    llvm.core.Type.int(16),                  # current_exception
    ))

continuation_th.type.refine(continuation)    
basic_frame_th.type.refine(basic_frame)    
    
# Returns the LLVM struct for a Continuation.
def structContinuation():
    return continuation
        
# Returns the LLVM struct for a BasicFrame.
def structBasicFrame():
    return basic_frame

#! /usr/bin/env
#
# LLVM code geneator.

import sys

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
import visitor

from codegen_utils import *

### Codegen visitor.

class CodeGen(visitor.Visitor):
    def __init__(self):
        super(CodeGen, self).__init__()
        self._infunction = False
        
        # Dummy class to create a sub-namespace.
        class llvm:
            pass
        
        self._llvm = llvm()
        self._llvm.module = None

    # Returns the final LLVM module once code-generation has finished.
    def llvmModule(self):
        try:
            self._llvm.module.verify()
        except llvm.LLVMException, e:
            util.error("LLVM error: %s" % e)
            
        return self._llvm.module
        
codegen = CodeGen()

### Overall control structures.

@codegen.when(module.Module)
def _(self, m):
    self._llvm.module = llvm.core.Module.new(m.name())

    self._llvm.module.add_type_name("__basic_frame", structBasicFrame())
    self._llvm.module.add_type_name("__continuation", structContinuation())
    
### Global ID definitions. 

@codegen.when(id.ID, type.StructDeclType)
def _(self, id):
    type = id.type().type()
    fields = [typeToLLVM(id.type()) for id in type.IDs()]
    struct = llvm.core.Type.struct(fields)

    self._llvm.module.add_type_name(id.name(), struct)
    
@codegen.when(id.ID, type.StorageType)
def _(self, id):
    if self._infunction:
        # Ignore locals, we do them when we're generating the function's code. 
        return
    
    self._llvm.module.add_global_variable(typeToLLVM(id.type()), id.name())

### Function definitions.

@codegen.pre(function.Function)
def _(self, f):
    self._infunction = True

@codegen.post(function.Function)
def _(self, f):
    self._infunction = False
    
### Block definitions.    
    
@codegen.pre(block.Block)
def _(self, b):
    pass
    
### Instructions.

# Generic instruction handler that shouldn't be reached because
# we write specialized handlers for all instructions.
@codegen.when(instruction.Instruction)
def _(self, i):
    #assert False, "instruction %s not handled in CodeGen" % i.name()
    pass

### Operands.
@codegen.when(instruction.IDOperand)
def _(self, i):
    #assert False, "instruction %s not handled in CodeGen" % i.name()
    pass

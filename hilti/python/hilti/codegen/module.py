# $Id$
#
# LLVM Code generation for module-level structure (module, struct, global, function)

import llvm
import llvm.core 

from hilti.core import *
from hilti import instructions
from codegen import codegen

@codegen.when(module.Module)
def _(self, m):
    self._module = m
    self._llvm.module = llvm.core.Module.new(m.name())
    self._llvm.module.add_type_name("__basic_frame", self.llvmTypeBasicFrame())
    self._llvm.module.add_type_name("__continuation", self.llvmTypeContinuation())
    
### Global ID definitions. 

@codegen.when(id.ID, type.StructDeclType)
def _(self, id):
    type = id.type().type()
    fields = [self.llvmTypeConvert(id.type()) for id in type.IDs()]
    struct = llvm.core.Type.struct(fields)

    self._llvm.module.add_type_name(id.name(), struct)
    
@codegen.when(id.ID, type.StorageType)
def _(self, id):
    if self._function:
        # Ignore locals, we do them when we're generating the function's code. 
        return
    
    self._llvm.module.add_global_variable(self.llvmTypeConvert(id.type()), id.name())

### Function definitions.

@codegen.pre(function.Function)
def _(self, f):
    self._function = f
    self._llvm.module.add_type_name(self.nameFunctionFrame(f), self.llvmTypeFunctionFrame(f))

@codegen.post(function.Function)
def _(self, f):
    self._function = None
    self._llvm.block_funcs = {}
    self._llvm.frameptr = None
    
### Block definitions.    

@codegen.pre(block.Block)
def _(self, b):
    assert self._function
    assert b.name()

    # Generate an LLVM function for this block.
    llvm_func = self.llvmGetFunctionForBlock(b)
    self._llvm.frameptr = llvm_func.args[0]
    llvm_block = llvm_func.append_basic_block("")
    self._llvm.builder = llvm.core.Builder.new(llvm_block)
    
    llvm_name = self.nameFunctionForBlock(b)
    self._llvm.functions[llvm_name] = llvm_func
    


# $Id$
#
# LLVM Code generation for module-level structure (module, struct, global, function)

import llvm
import llvm.core 

from hilti.core import *
from hilti import instructions
from codegen import codegen

import sys

@codegen.when(module.Module)
def _(self, m):
    self._module = m
    self._llvm.module = codegen.llvmNewModule(m.name())
    self._llvm.module.add_type_name("__basic_frame", self.llvmTypeBasicFrame())
    self._llvm.module.add_type_name("__continuation", self.llvmTypeContinuation())
    self.llvmAddTypeInfos()
    
### Global ID definitions. 

@codegen.when(id.ID, type.StructDecl)
def _(self, id):
    type = id.type().declType()
    fields = [self.llvmTypeConvert(i.type()) for (i, o) in type.Fields()]
    struct = llvm.core.Type.struct(fields)

    self.llvmCurrentModule().add_type_name(id.name(), struct)
    
@codegen.when(id.ID, type.StorageType)
def _(self, id):
    if self.currentFunction():
        # Ignore locals, we do them when we're generating the function's code. 
        return
    
    self.llvmCurrentModule().add_global_variable(self.llvmTypeConvert(id.type()), id.name())

### Function definitions.

@codegen.pre(function.Function)
def _(self, f):
    self._function = f
    self._label_counter = 0
    self.llvmCurrentModule().add_type_name(self.nameFunctionFrame(f), self.llvmTypeFunctionFrame(f))

@codegen.post(function.Function)
def _(self, f):
    self._function = None
    self._llvm.frameptr = None
    
### Block definitions.    

@codegen.pre(block.Block)
def _(self, b):
    assert self.currentFunction()
    assert b.name()

    self._block = b
    
    self._llvm.func = self.llvmGetFunctionForBlock(b)
    self._llvm.frameptr = self._llvm.func.args[0]
    self._llvm.block = self._llvm.func.append_basic_block("")
    self.pushBuilder(self._llvm.block)
    
@codegen.post(block.Block)
def _(self, b):
    self._llvm.func = None
    self._llvm.block = None
    self._block = None
    self.popBuilder()

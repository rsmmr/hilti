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
    self._llvm.module = codegen.llvmNewModule(m.name())
    self._llvm.module.add_type_name("__basic_frame", self.llvmTypeBasicFrame())
    self._llvm.module.add_type_name("__continuation", self.llvmTypeContinuation())
    self.llvmAddTypeInfos()
    
### Global ID definitions. 

@codegen.when(id.ID, type.ValueType)
def _(self, i):
    if self.currentFunction():
        # Ignore locals, we do them when we're generating the function's code. 
        return

    if i.role() == id.Role.GLOBAL:
        self.llvmCurrentModule().add_global_variable(self.llvmTypeConvert(i.type()), i.name())

### Function definitions.

@codegen.pre(function.Function)
def _(self, f):
    self._function = f
    self._label_counter = 0
    
    if f.callingConvention() == function.CallingConvention.HILTI:
        self.llvmCurrentModule().add_type_name(self.nameFunctionFrame(f), self.llvmTypeFunctionFrame(f))
    
    if f.linkage() == function.Linkage.EXPORTED and f.callingConvention() == function.CallingConvention.HILTI:
        codegen.llvmCStubs(f)
        
    if f.linkage() == function.Linkage.INIT:
        def _makeCall():
            self.llvmGenerateCCall(f, [], [], abort_on_except=True)
        
        codegen.llvmAddGlobalCtor(_makeCall)

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
    if self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
        self._llvm.frameptr = self._llvm.func.args[0]
    else:
        self._llvm.frameptr = None
        
    self._llvm.block = self._llvm.func.append_basic_block("")
    self.pushBuilder(self._llvm.block)
    
@codegen.post(block.Block)
def _(self, b):
    self._llvm.func = None
    self._llvm.block = None
    self._block = None
    self.popBuilder()

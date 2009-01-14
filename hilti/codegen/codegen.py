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

import ins

from codegen_utils import *

### Codegen visitor.

class CodeGen(visitor.Visitor):
    def __init__(self):
        super(CodeGen, self).__init__()
        self._function = None
        
        # Dummy class to create a sub-namespace.
        class llvm:
            pass
        
        self._llvm = llvm()
        self._llvm.module = None

    # Returns the final LLVM module once code-generation has finished.
    def llvmModule(self, fatalerrors=True):
        try:
            self._llvm.module.verify()
            pass
        except llvm.LLVMException, e:
            if fatalerrors:
                util.error("LLVM error: %s" % e)
            else:
                util.warning("LLVM error: %s" % e)
            
        return self._llvm.module
    
codegen = CodeGen()

### Overall control structures.

@codegen.when(module.Module)
def _(self, m):
    self._module = m
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
    if self._function:
        # Ignore locals, we do them when we're generating the function's code. 
        return
    
    self._llvm.module.add_global_variable(typeToLLVM(id.type()), id.name())

### Function definitions.

@codegen.pre(function.Function)
def _(self, f):
    self._function = f
    self._llvm.module.add_type_name(nameFunctionFrame(f), structFunctionFrame(f))
    self._llvm.functions = {}

@codegen.post(function.Function)
def _(self, f):
    self._function = None
    
    self._llvm.function = None
    self._llvm.functions = None
    self._llvm.frameptr = None
    
### Block definitions.    

# Returns LLVM function for the given block name, creating one if it doesn't exist yet. 
def _getFunction(cg, name):
    try:
        return cg._llvm.functions[name]
    except KeyError:
        
        rt = typeToLLVM(cg._function.type().resultType())
        ft = llvm.core.Type.function(rt, [llvm.core.Type.pointer(structFunctionFrame(cg._function))])
        func = cg._llvm.module.add_function(ft, blockFunctionName(name, cg._function))
        func.args[0].name = "__frame"
        cg._llvm.functions[name] = func
        return func

# Generates LLVM tail call code. From the LLVM 1.5 release notes:
#
#    To ensure a call is interpreted as a tail call, a front-end must
#    mark functions as "fastcc", mark calls with the 'tail' marker,
#    and follow the call with a return of the called value (or void).
#    The optimizer and code generator attempt to handle more general
#    cases, but the simple case will always work if the code
#    generator supports tail calls. Here is an example:
#
#    fastcc int %bar(int %X, int(double, int)* %FP) {       ; fastcc
#        %Y = tail call fastcc int %FP(double 0.0, int %X)  ; tail, fastcc
#        ret int %Y
#    }
def _genTailCall(cg, funcname, args):
    llvm_func = _getFunction(cg, funcname)
    if cg._function.type().resultType() == type.Void:
        cg._llvm.builder.call(llvm_func, args)
        cg._llvm.builder.ret_void()
    else:
        result = cg._llvm.builder.call(llvm_func, args, "result")
        result.calling_convention = llvm.core.CC_FASTCALL
        cg._llvm.builder.ret(result)
        
    # FIXME: How do we get the "tail" modifier in there?
    
    
@codegen.pre(block.Block)
def _(self, b):
    assert self._function
    assert b.name()

    # Generate an LLVM function for this block.
    self._llvm.function = _getFunction(self, b.name())
    self._llvm.frameptr = self._llvm.function.args[0]
    
    block = self._llvm.function.append_basic_block("")
    self._llvm.builder = llvm.core.Builder.new(block)

#tmp = builder.add(f_sum.args[0], f_sum.args[1], "tmp")
    
### Instructions.

# Generic instruction handler that shouldn't be reached because
# we write specialized handlers for all instructions.
@codegen.when(instruction.Instruction)
def _(self, i):
    #assert False, "instruction %s not handled in CodeGen" % i.name()
    pass

@codegen.when(ins.flow.ReturnVoid)
def _(self, i):
    self._llvm.builder.ret_void()
        
@codegen.when(ins.flow.ReturnResult)
def _(self, i):
    self._llvm.builder.ret(opToLLVM(self, i.op1(), "result"))

@codegen.when(ins.flow.Jump)
def _(self, i):
    _genTailCall(self, i.op1().value(), [self._llvm.frameptr])

### Operands.
@codegen.when(instruction.IDOperand)
def _(self, i):
    #assert False, "instruction %s not handled in CodeGen" % i.name()
    pass

## Integer
@codegen.when(ins.integer.Add)
def _(self, i):
    op1 = opToLLVM(self, i.op1(), "op1")
    op2 = opToLLVM(self, i.op2(), "op2")
    result = self._llvm.builder.add(op1, op2, "tmp")
    targetToLLVM(self, i.target(), result, "target")

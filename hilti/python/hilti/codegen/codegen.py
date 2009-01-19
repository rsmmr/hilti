#! /usr/bin/env
#
# LLVM code geneator.
"""
General Conventions
~~~~~~~~~~~~~~~~~~~

* Identifiers starting with ``__`` are reservered for internal
  purposes.
  
Data Structures
~~~~~~~~~~~~~~~

* A ''__continuation'' is a snapshot of the current processing state,
  consisting of (a) a function to be called to continue
  processing, and (b) the frame to use when calling the function.::

    struct Continuation {
        ref<label>         func
        ref<__basic_frame> frame
    }

* A ''__basic_frame'' is the common part of all function frame::

    struct __basic_frame {
        ref<__continuation> cont_normal 
        ref<__continuation> cont_exception
        int                 current_exception
        ref<any>            current_exception_data
    }
    
* Each function ''Foo'' has a function-specific frame containg all
  parameters and local variables::

    struct ''__frame_Foo'' {
        __basic_frame  bf
        <type_1>       arg_1
        ...
        <type_n>       arg_n
        <type_n+1>     local_1
        ...
        <type_n+m>     local_m
        

Calling Conventions
~~~~~~~~~~~~~~~~~~~

* All functions take a mandatory first parameter
  ''ref<__frame__Func> __frame''. Functions which work on the result
  of another function call (see below), take a second parameter
  ''<result-type> __return_value''.
  
"""

import sys

import llvm
import llvm.core 

from hilti.core import *
from hilti import ins

### Codegen visitor.

class CodeGen(visitor.Visitor):
    def __init__(self):
        super(CodeGen, self).__init__()
        self.reset()
        
    def reset(self):
        # Dummy class to create a sub-namespace.
        class _llvm_data:
            pass
        
        self._function = None
        self._llvm = _llvm_data()
        self._llvm.module = None
        self._llvm.block_funcs = {}
        self._llvm.type_generic_pointer = llvm.core.Type.int(8)
        self._llvm.int_consts = []
        self._llvm.frameptr = None
        self._llvm.functions = {}
        
        self._llvm.type_continuation = llvm.core.Type.struct([
        	self._llvm.type_generic_pointer, # Successor function
            self._llvm.type_generic_pointer  # Frame pointer
            ])
        
        self._llvm.type_basic_frame = llvm.core.Type.struct([
        	self._llvm.type_continuation,     # Default continuation 
        	self._llvm.type_continuation,     # Exceptional continuation 
		    llvm.core.Type.int(16),           # Current exception
            self._llvm.type_generic_pointer,  # Current exception's data.
            ])
        
    def builder(self):
        return self._llvm.builder

    # Looksup the function belonging to the given name. Returns None if not found. 
    # FIXME: Not sure this belongs here. We need to lookup functions from other modules as well.
    def lookupFunction(self, name):
        return self._module.function(name)

    # Returns the internal LLVM name for the given function.
    # FIXME: Integrate module name.
    def nameFunction(self, function):
        return function.name()
    
    # Returns the internal function name for the given block.
    # FIXME: Integrate module name.
    def nameFunctionForBlock(self, block):
        function = block.function()
        name = block.name()
        
        first_block = function.blocks()[0].name()
        if first_block.startswith("@"):
            first_block = first_block[1:]

        if name.startswith("@"):
            name = name[1:]
        
        if name == first_block:
            return self.nameFunction(function)
        
        if name.startswith("__"):
            name = name[2:]
    
        return "__%s_%s" % (function.name(), name)

    def nameFunctionFrame(self, function):
        # FIXME: include module name
        return "__frame_%s" % function.name()
    
    # Returns the final LLVM module once code-generation has finished.
    def llvmModule(self, verify=True):
        try:
            if verify:
                if not self._llvm.module.verify():
                    return None
                
        except llvm.LLVMException, e:
            util.warning("LLVM error: %s" % e)
            
        return self._llvm.module

    # Return the LLVM constant representing the given integer.
    def llvmConstInt(self, n):
        try:
            return self._llvm.int_consts[n]
        except IndexError:
            self._llvm.int_consts += [llvm.core.Constant.int(llvm.core.Type.int(), i) for i in range(len(self._llvm.int_consts), n + 1)]
            return self._llvm.int_consts[n]            

    # Returnt he LLVM constant representing the given exception type (which is an int).
    def llvmConstException(self, n):
        return llvm.core.Constant.int(llvm.core.Type.int(16), n)
        
    # Returns the type used to represent pointer of different type. 
    # (i.e., "void *")
    def llvmTypeGenericPointer(self):
        return self._llvm.type_generic_pointer

    # Returns a type representing a pointer to the given function. 
    def llvmTypeFunctionPtr(self, function):
        rt = self.llvmTypeConvert(function.type().resultType())
        ft = llvm.core.Type.function(rt, [llvm.core.Type.pointer(self.llvmTypeFunctionFrame(function))])
        return llvm.core.Type.pointer(llvm.core.Type.pointer(ft))

    # Returns a type representing a pointer to a function receiving only 
    # __basic_frame as parameter (and returning void).
    def llvmTypeBasicFunctionPtr(self):
        voidt = llvm.core.Type.void()
        ft = llvm.core.Type.function(voidt, [llvm.core.Type.pointer(self.llvmTypeBasicFrame())])
        return llvm.core.Type.pointer(llvm.core.Type.pointer(ft))
    
    # Returns type of continuation structure.
    def llvmTypeContinuation(self):
        return self._llvm.type_continuation
    
    # Returns type of basic frame structure.
    def llvmTypeBasicFrame(self):
        return self._llvm.type_basic_frame

    # Returns the LLVM for the function's frame.
    def llvmTypeFunctionFrame(self, function):
        fields = [("__bf", self.llvmTypeBasicFrame())]
        fields += [(id.name(), self.llvmTypeConvert(id.type())) for id in function.scope().IDs().values()]
    
        # Build map of indices and store with function.
        function._frame_index = {}
        for i in range(0, len(fields)):
            function._frame_index[fields[i][0]] = i

        frame = [f[1] for f in fields]
        return llvm.core.Type.struct(frame)

    def _llvmAddrInBasicFrame(self, frame, index1, index2, cast_to, tag):
        zero = self.llvmConstInt(0)
        index1 = self.llvmConstInt(index1)
        
        if index2 >= 0:
            index2 = self.llvmConstInt(index2)
            index = [zero, zero, index1, index2]
        else:
            index = [zero, zero, index1]
        
        if not cast_to:
            addr = self.builder().gep(frame, index, tag)
            return addr
        else:
            addr = self.builder().gep(frame, index, "tmp" + tag)
            return self.builder().bitcast(addr, cast_to, tag)
    
    # Returns the address of the default continuation's successor field in the given frame.
    def llvmAddrDefContSuccessor(self, frame, cast_to = None):
        return self._llvmAddrInBasicFrame(frame, 0, 0, cast_to, "cont_succ")
    
    # Returns the address of the default continuation's frame field in the given frame.
    def llvmAddrDefContFrame(self, frame, cast_to = None):
        return self._llvmAddrInBasicFrame(frame, 0, 1, cast_to, "cont_frame")

    # Returns the address of the exception continuation's successor field in the given frame.
    def llvmAddrExceptContSuccessor(self, frame, cast_to = None):
        return self._llvmAddrInBasicFrame(frame, 1, 0, cast_to, "excep_succ")
    
    # Returns the address of the exception continuation's frame field in the given frame.
    def llvmAddrExceptContFrame(self, frame, cast_to = None):
        return self._llvmAddrInBasicFrame(frame, 1, 1, cast_to, "excep_frame")

    # Returns the address of the current exception field in the given frame.
    def llvmAddrException(self, frame):
        return self._llvmAddrInBasicFrame(frame, 2, -1, None, "excep_frame")
    
    # Returns the address of the current exception's data field in the given frame.
    def llvmAddrExceptionData(self, frame, cast_to=None):
        return self._llvmAddrInBasicFrame(frame, 3, -1, cast_to, "excep_data")
    
    # Returns LLVM function for the given block, creating one if it doesn't exist yet. 
    def llvmGetFunctionForBlock(self, block):
        name = self.nameFunctionForBlock(block) 
        
        try:
            return self._llvm.block_funcs[name]
        except KeyError:
            rt = self.llvmTypeConvert(block.function().type().resultType())
            args = [llvm.core.Type.pointer(self.llvmTypeFunctionFrame(block.function()))]
            ft = llvm.core.Type.function(rt, args)
            func = self._llvm.module.add_function(ft, self.nameFunctionForBlock(block))
            func.args[0].name = "__frame"
            self._llvm.block_funcs[name] = func
            return func

    # Returns the LLVM function for the given function if it was already created, and None otherwise.
    def llvmFunction(self, function):
        try:
            name = self.nameFunction(function)
            return self._llvm.functions[name]
        except KeyError:
            return None
        
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
    #
    # FIXME: How do we get the "tail" modifier in there?
    
    def _llvmGenerateTailCall(self, llvm_func, args):
        result = self.builder().call(llvm_func, args)
        result.calling_convention = llvm.core.CC_FASTCALL
        self.builder().ret_void()

#    def _llvmGenerateTailCall(self, llvm_func, args):
#        if function.type().resultType() == type.Void:
#            self.builder().call(llvm_func, args)
#            self.builder().ret_void()
#        else:
#            result = self.builder().call(llvm_func, args, "result")
#            result.calling_convention = llvm.core.CC_FASTCALL
#            self.builder().ret(result)    
            
            
    def llvmGenerateTailCallToBlock(self, block, args):
        llvm_func = self.llvmGetFunctionForBlock(block)
        self._llvmGenerateTailCall(llvm_func, args)
            
    def llvmGenerateTailCallToFunction(self, function, args):
        llvm_func = self.llvmFunction(function)
        assert llvm_func
        self._llvmGenerateTailCall(llvm_func, args)

    def llvmGenerateTailCallToFunctionPtr(self, ptr, args):
        self._llvmGenerateTailCall(ptr, args)
        
    # Converts a StorageType into the corresponding type for LLVM variable declarations.
    # FIXME: Don't hardcode types here.
    def llvmTypeConvert(self, t):
        mapping = {
        	type.String: llvm.core.Type.array(llvm.core.Type.int(8), 0),
            type.Bool:   llvm.core.Type.int(1), 
            }
            
        if isinstance(t, type.IntegerType):
            return llvm.core.Type.int(t.width())
    
        if isinstance(t, type.VoidType):
            return llvm.core.Type.void()
    
        try:
            return mapping[t]
        except KeyError:
            util.internal_error("codegen: cannot translate type %s" % t.name())    

    # Turns an operand into LLVM speak.
    # FIXME: Don't hardcode operand types here (or?)
    def llvmOp(self, op, tag):
        if isinstance(op, instruction.ConstOperand):
            return self.llvmConstOp(op)

        if isinstance(op, instruction.IDOperand):
            if op.isLocal():
                # The Python interface (as well as LLVM's C interface, which is used
                # by the Python interface) does not have builder.extract_value() method,
                # so we simulate it with a gep/load combination.
                try:
                    frame_idx = self._function._frame_index
                except AttributeError:
                    # Not yet calculated.
                    llvmTypeFunctionFrame()
                    frame_idx = self._function._frame_index
                
                zero = self.llvmConstInt(0)
                index = self.llvmConstInt(frame_idx[op.value()])
                addr = self.builder().gep(self._llvm.frameptr, [zero, index], tag)
                return self.builder().load(addr, tag)
            
        util.internal_error("opToLLVM: unknown op class: %s" % op)

    def llvmConstOp(self, op):
        # FIXME: Don't hardcode operand types here
        if isinstance(op.type(), type.IntegerType):
            return llvm.core.Constant.int(self.llvmTypeConvert(op.type()), op.value())
            
        if isinstance(op.type(), type.StringType):
            return llvm.core.Constant.string(op.value())
        
        util.internal_error("constOpToLLVM: illegal type %s" % op.type())
        
    # Acts like a builder.store() yet signals that the assignment
    # initializes the address for the first time, there hasn't been anything else
    # stored at addr previously.
    # 
    # Currently, this indeed just turns into a builder.store(), however if we eventually
    # turn to reference counting, this will bne adapted accordingly. 
    def llvmInit(self, value, addr):
        self.builder().store(value, addr)
        
    # Acts like a builder.store() yet signals that the assignment
    # overrides an address that might have stored a value previously already. 
    # 
    # Currently, this indeed just turns into a builder.store(), however if we eventually
    # turn to reference counting, this will bne adapted accordingly. 
    #
    # FIXME: Function call/returns don't call these yet. 
    def llvmAssign(self, value, addr):
        self.builder().store(value, addr)
        
    
    # Acts like builder.malloc() but might in the future do more to 
    # faciliate garbage collection.
    def llvmMalloc(self, type, tag):
        return self.builder().malloc(type, tag)
        
    # Stores value in target.
    def llvmStoreInTarget(self, target, val, tag):
        # FIXME: Don't hardcode operand types here (or?)
        if isinstance(target, instruction.IDOperand):
            if target.isLocal():
                # The Python interface (as well as LLVM's C interface, which is used
                # by the Python interface) does not have builder.extract_value() method,
                # so we simulate it with a gep/load combination.
                try:
                    frame_idx = self._function._frame_index
                except AttributeError:
                    # Not yet calculated.
                    llvmTypeFunctionFrame()
                    frame_idx = self._function._frame_index
                
                zero = self.llvmConstInt(0)
                index = self.llvmConstInt(frame_idx[target.value()])
                addr = self.builder().gep(self._llvm.frameptr, [zero, index], tag)
                self.llvmAssign(val, addr)
                return 
                
        util.internal_error("targetToLLVM: unknown target class: %s" % target)
            
##############            
            
codegen = CodeGen()

def generate(ast, verify=True):
    """Compiles *ast* into LLVM module. *ast* must have been already canonified."""
    codegen.reset()
    codegen.dispatch(ast)
    return codegen.llvmModule(verify)

### Overall control structures.

    
# Generic instruction handler that shouldn't be reached because
# we write specialized handlers for all instructions.
@codegen.when(instruction.Instruction)
def _(self, i):
    #assert False, "instruction %s not handled in CodeGen" % i.name()
    pass


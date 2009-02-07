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

* An exception is just call of the current exception continuation (which must
  be non-null at all times.)
  
"""

import sys

import llvm
import llvm.core 

from hilti.core import *
from hilti import instructions

# Class which just absorbs any method calls.
class _DummyBuilder(object):
    def __getattribute__(*args):
        class dummy:
            pass
        return lambda *args: dummy()
    
### Codegen visitor.

class CodeGen(visitor.Visitor):
    def __init__(self):
        super(CodeGen, self).__init__()
        self.reset()
        
    def reset(self):
        # Dummy class to create a sub-namespace.
        class _llvm_data:
            pass

        self.ret_func_counter = 0
        self.label_counter = 0
        
        self._function = None
        self._llvm = _llvm_data()
        self._llvm.module = None
        self._llvm.block_funcs = {}
        self._llvm.c_funcs = {}
        self._llvm.type_generic_pointer = llvm.core.Type.pointer(llvm.core.Type.int(8))
        self._llvm.int_consts = []
        self._llvm.frameptr = None
        self._llvm.functions = {}
        self._llvm.builders = []
        
        self._llvm.type_continuation = llvm.core.Type.struct([
        	self._llvm.type_generic_pointer, # Successor function
            self._llvm.type_generic_pointer  # Frame pointer
            ])

        self._bf_fields = [
        	("cont_normal", self._llvm.type_continuation), 
        	("cont_except", self._llvm.type_continuation),    
		    ("exception", self._llvm.type_generic_pointer),
            ("exception_data", self._llvm.type_generic_pointer)
            ]

        self._llvm.type_basic_frame = llvm.core.Type.struct([ t for (n, t) in self._bf_fields])

    def generateLLVM(self, ast, verify=True):
        self.reset()
        self.dispatch(ast)
        return self.llvmModule(verify)
            
    def pushBuilder(self, llvm_block):
        builder = llvm.core.Builder.new(llvm_block) if llvm_block else _DummyBuilder()
        self._llvm.builders += [builder]
        
    def popBuilder(self):
        self._llvm.builders = self._llvm.builders[0:-1]
    
    def builder(self):
        return self._llvm.builders[-1]

    # Looks up the function belonging to the given name. Returns None if not found. 
    def lookupFunction(self, id):
        func = self._module.lookupGlobal(id.name())
        assert (not func) or (isinstance(func.type(), type.FunctionType))
        return func
    
    # Returns the internal LLVM name for the given function.
    def nameFunction(self, func):
        name = func.name().replace("::", "_")
        
        if func.callingConvention() == function.CallingConvention.C:
            # Don't mess with the names of C functions.
            return name
        
        if func.callingConvention() == function.CallingConvention.HILTI:
            return "hlt_%s" % name
        
        # Cannot be reached
        assert False
            
    
    # Returns the internal function name for the given block.
    # FIXME: Integrate module name.
    def nameFunctionForBlock(self, block):
        function = block.function()
        funcname = self.nameFunction(function)
        name = block.name()
        
        first_block = function.blocks()[0].name()
        if first_block.startswith("@"):
            first_block = first_block[1:]

        if name.startswith("@"):
            name = name[1:]
        
        if name == first_block:
            return funcname
        
        if name.startswith("__"):
            name = name[2:]
    
        return "__%s_%s" % (funcname, name)

    # Returns a new label guaranteed to be unique within the current function.
    def nameNewLabel(self, postfix):
        self.label_counter += 1
        return "l%d-%s" % (self.label_counter, postfix)
    
    # Returns a new block with a label guaranteed to be unique within the
    # current function.
    def llvmNewBlock(self, postfix):
        return self._llvm.func.append_basic_block(self.nameNewLabel(postfix))

    def nameFunctionFrame(self, function):
        return "__frame_%s" % self.nameFunction(function)
    
    # Returns the final LLVM module once code-generation has finished.
    def llvmModule(self, verify=True):
        
        if verify:
            try:
                self._llvm.module.verify()
            except llvm.LLVMException, e:
                util.error("LLVM error: %s" % e, fatal=False)
                return (False, None)
            
        return (True, self._llvm.module)

    # Return the LLVM constant representing the given integer.
    def llvmConstInt(self, n, width = 32):
        return llvm.core.Constant.int(llvm.core.Type.int(width), n)

    # Returns the LLVM constant representing an unset exception.
    def llvmConstNoException(self):
        return llvm.core.Constant.null(self.llvmTypeGenericPointer())

    # Returns the type used to represent pointer of different type. 
    # (i.e., "void *")
    def llvmTypeGenericPointer(self):
        return self._llvm.type_generic_pointer

    # Returns a type representing a pointer to the given function. 
    def llvmTypeFunctionPtr(self, function):
        voidt = llvm.core.Type.void()
        ft = llvm.core.Type.function(voidt, [llvm.core.Type.pointer(self.llvmTypeFunctionPtrFrame(function))])
        return llvm.core.Type.pointer(llvm.core.Type.pointer(ft))

    # Returns a type representing a pointer to a function receiving 
    # __basic_frame as parameter (and returning void). If additional args
    # types are given, they are added to the function's signature.
    def llvmTypeBasicFunctionPtr(self, args=[]):
        voidt = llvm.core.Type.void()
        ft = llvm.core.Type.function(voidt, [llvm.core.Type.pointer(self.llvmTypeBasicFrame())] + [self.llvmTypeConvert(t) for t in args])
        return llvm.core.Type.pointer(llvm.core.Type.pointer(ft))
    
    # Returns type of continuation structure.
    def llvmTypeContinuation(self):
        return self._llvm.type_continuation
    
    # Returns type of basic frame structure.
    def llvmTypeBasicFrame(self):
        return self._llvm.type_basic_frame

    # Returns the LLVM for the function's frame.
    def llvmTypeFunctionFrame(self, function):
        
        try:
            # Is it cached?
            if function._llvm_frame != None:
                return function._llvm_frame
        except AttributeError:
            pass
        
        fields = self._bf_fields
        ids = function.type().IDs() + function.IDs().values()
        fields = fields + [(id.name(), self.llvmTypeConvert(id.type())) for id in ids]
        
        # Build map of indices and store with function.
        function._frame_index = {}
        for i in range(0, len(fields)):
            function._frame_index[fields[i][0]] = i

        frame = [f[1] for f in fields]
        function._llvm_frame = llvm.core.Type.struct(frame) # Cache it.
        
        return function._llvm_frame

    def _llvmAddrInBasicFrame(self, frame, index1, index2, cast_to, tag):
        zero = self.llvmConstInt(0)
        index1 = self.llvmConstInt(index1)
        
        if index2 >= 0:
            index2 = self.llvmConstInt(index2)
            index = [zero, index1, index2]
        else:
            index = [zero, index1]
        
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
    
    # Returns the address of the local variable in the given frame.
    # V is given via it's string name. 
    def llvmAddrLocalVar(self, function, frameptr, id, tag):
        # The Python interface (as well as LLVM's C interface, which is used
        # by the Python interface) does not have builder.extract_value() method,
        # so we simulate it with a gep/load combination.
        try:
            frame_idx = function._frame_index
        except AttributeError:
            # Not yet calculated.
            self.llvmTypeFunctionFrame(function)
            frame_idx = function._frame_index
            
        zero = self.llvmConstInt(0)
        index = self.llvmConstInt(frame_idx[id.name()])
        return self.builder().gep(frameptr, [zero, index], tag)

    # Loads the value of the given external global. 
    def llvmGetExternalGlobal(self, type, name):
        
        try:
            glob = self._llvm.module.get_global_variable_named(name)
            # FIXME: Check that the type is the same.
            return self.builder().load(glob, "exception")
        except llvm.LLVMException:
            pass
        
        glob = self._llvm.module.add_global_variable(type, name)
        glob.linkage = llvm.core.LINKAGE_EXTERNAL 
        return self.builder().load(glob, "exception")

    # Creates a new function matching the block's frame/result. Additional
    # arguments to the function can be specified by passing (name, type)
    # tuples via addl_args.
    def llvmCreateFunctionForBlock(self, name, block, addl_args = []):
        
        # Build function type.
        args = [llvm.core.Type.pointer(self.llvmTypeFunctionFrame(block.function()))]
        ft = llvm.core.Type.function(llvm.core.Type.void(), args + [t for (n, t) in addl_args]) 
        
        # Create function.
        func = self._llvm.module.add_function(ft, name)
        func.calling_convention = llvm.core.CC_FASTCALL
        if name.startswith("__"):
            func.linkage = llvm.core.LINKAGE_INTERNAL
        
        # Set parameter names. 
        func.args[0].name = "__frame"
        for i in range(len(addl_args)):
            func.args[i+1].name = addl_args[i][0]
        
        self._llvm.block_funcs[name] = func

        return func
    
    # Returns LLVM function for the given block, creating one if it doesn't exist yet. 
    def llvmGetFunctionForBlock(self, block):
        name = self.nameFunctionForBlock(block) 
        try:
            return self._llvm.block_funcs[name]
        except KeyError:
            return self.llvmCreateFunctionForBlock(name, block)

    # Returns LLVM for external C function.
    def llvmGetCFunction(self, function):
        name = function.name().replace("::", "_")
        
        try:
            return self._llvm.c_funcs[name]
        except KeyError:
            rt = self.llvmTypeConvert(function.type().resultType())
            args = [self.llvmTypeConvertToC(id.type()) for id in function.type().IDs()]
            ft = llvm.core.Type.function(rt, args)
            func = self._llvm.module.add_function(ft, name)
            self._llvm.c_funcs[name] = func
            return func

    # Generates a call to a C function.
    # FIXME: If the LLVM type differs from the one supposed to be used
    # for the C call, we don't convert yet and thus will likely get
    # a "bad signature" assertion. See llvmOpToC.
    def llvmGenerateCCall(self, function, args):
        llvm_func = self.llvmGetCFunction(function)
        
        tag = None
        if function.type().resultType() == type.Void:
            call = self.builder().call(llvm_func, args)
        else:
            call = self.builder().call(llvm_func, args, "result")
        
        call.calling_convention = llvm.core.CC_C

    # Raises the named exception. The is the corresponding C name from
    # libhilti/execeptions.cc.
    def llvmRaiseException(self, exception):
        
        # ptr = *__frame.bf.cont_exception.succesor
        fpt = self.llvmTypeBasicFunctionPtr()
        addr = self.llvmAddrExceptContSuccessor(self._llvm.frameptr, fpt)
        ptr = self.builder().load(addr, "succ_ptr")
    
        # frame = *__frame.bf.cont_exception
        bfptr = llvm.core.Type.pointer(self.llvmTypeBasicFrame())
        addr = self.llvmAddrExceptContFrame(self._llvm.frameptr, llvm.core.Type.pointer(bfptr))
        frame = self.builder().load(addr, "frame_ptr")
    
        # frame.exception = <exception>
        addr = self.llvmAddrException(frame)
        exc = self.llvmGetExternalGlobal(self.llvmTypeGenericPointer(), exception)
        self.llvmInit(exc, addr)
        
        # frame.exception_data = null (for now)
        addr = self.llvmAddrExceptionData(frame)
        null = llvm.core.Constant.null(self.llvmTypeGenericPointer())
        self.llvmInit(null, addr)
    
        self.llvmGenerateTailCallToFunctionPtr(ptr, [frame])

    # Creates a new LLVM function for the function. 
    def llvmCreateFunctionForFunction(self, function):
        name = self.nameFunction(function)
        
        # Build function type.
        args = [llvm.core.Type.pointer(self.llvmTypeFunctionFrame(function))]
        ft = llvm.core.Type.function(llvm.core.Type.void(), args)
        
        # Create function.
        func = self._llvm.module.add_function(ft, name)
        func.calling_convention = llvm.core.CC_FASTCALL
        if name.startswith("__"):
            func.linkage = llvm.core.LINKAGE_INTERNAL
        
        # Set parameter names. 
        func.args[0].name = "__frame"
        self._llvm.functions[name] = func
        return func
        
    # Returns the LLVM function for the given function if it was already
    # created; creates it otherwise.
    def llvmFunction(self, function):
        try:
            name = self.nameFunction(function)
            return self._llvm.functions[name]
        except KeyError:        
            return self.llvmCreateFunctionForFunction(function)
        
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

        # Any subsequent code generated inside the same block will
        # be  unreachable. We create a dummy builder to absorb it; the code will
        # not appear anywhere in the output.
        self.popBuilder();
        self.pushBuilder(None)
        
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
        
    # Turns an operand into LLVM speak.
    # Can only be called when working with a builder for the current function. 
    #
    # We do only a very limited amount of casting here, currently only:
    #    - Integer constants which don't have a width can be casted to one
    #      with a width.
    
    def llvmOp(self, op, tag, cast_to_type=None):
        if isinstance(op, instruction.ConstOperand):
            return self.llvmConstOp(op, cast_to_type=cast_to_type)

        assert not cast_to_type or cast_to_type == op.type() # Cast not possible
        
        if isinstance(op, instruction.IDOperand):
            i = op.id()
            if i.role() == id.Role.LOCAL or i.role() == id.Role.PARAM:
                # A local variable.
                addr = self.llvmAddrLocalVar(self._function, self._llvm.frameptr, i, tag)
                return self.builder().load(addr, tag)
            
        util.internal_error("opToLLVM: unknown op class: %s" % op)

    def llvmConstOp(self, op, cast_to_type):

        if cast_to_type and isinstance(cast_to_type, type.IntegerType) and \
           isinstance(op.type(), type.IntegerType) and \
           op.type().width() == 0 and cast_to_type.width() != 0:
               return llvm.core.Constant.int(llvm.core.Type.int(cast_to_type.width()), op.value())

        assert not cast_to_type or cast_to_type == op.type() # Cast not possible
           
        # By now, we can't have any more integers of unspecified size.
        assert not isinstance(op.type(), type.IntegerType) or op.type().width() > 0

        if isinstance(op.type(), type.IntegerType):
            return llvm.core.Constant.int(llvm.core.Type.int(op.type().width()), op.value())
        
        if isinstance(op.type(), type.StringType):
            return llvm.core.Constant.string(op.value())
        
        util.internal_error("constOpToLLVM: illegal type %s" % op.type())

    # Turns an operand into LLVM speak suitable for passing to a C function.
    # FIXME: We need to do the conversion.
    def llvmOpToC(self, op, tag, cast_to_type=None):
        return self.llvmOp(op, tag, cast_to_type)
        
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
    #
    # Can only be used when working with a builder for the current function. 
    def llvmStoreInTarget(self, target, val, tag):
        if isinstance(target, instruction.IDOperand):
            i = target.id()
            if i.role() == id.Role.LOCAL or i.role() == id.Role.PARAM:
                # A local variable.
                addr = self.llvmAddrLocalVar(self._function, self._llvm.frameptr, i, tag)
                self.llvmAssign(val, addr)
                return 
                
        util.internal_error("targetToLLVM: unknown target class: %s" % target)

    # Table of callbacks for type conversions. 
    _CONV_TYPE_TO_LLVM = 1
    _CONV_TYPE_TO_LLVM_C = 2
    
    _Conversions = { _CONV_TYPE_TO_LLVM: [], _CONV_TYPE_TO_LLVM_C: [] }

    # Converts a StorageType into the corresponding type for LLVM variable declarations.
    def llvmTypeConvert(self, type):
        for (t, func) in CodeGen._Conversions[CodeGen._CONV_TYPE_TO_LLVM]:
            if isinstance(type, t):
                return func(type)
            
        util.internal_error("llvmConvertType: cannot translate type %s" % type.name())

    # Converts a StorageType into the corresponding type for passing to a C function. 
    def llvmTypeConvertToC(self, type):
        for (t, func) in CodeGen._Conversions[CodeGen._CONV_TYPE_TO_LLVM_C]:
            if isinstance(type, t):
                return func(type)

        # Fall back as default.
        return self.llvmTypeConvert(type)

    ### Decorators.
    
    # Decorator @convertToLLVM(type) to define a conversion from a StorageType
    # to the corresponding LLVM type.
    def convertToLLVM(self, type):
        def register(func):
            CodeGen._Conversions[CodeGen._CONV_TYPE_TO_LLVM] += [(type, func)]
    
        return register
    
    # Decorator @convertToC(type) to define a conversion from a StorageType
    # to the  LLVM type used for passing it to the C functions. The default,
    # if nothing is defined for a type, is to fall back on @convertToLLVM.
    def convertToC(self, type):
        def register(func):
            CodeGen._Conversions[CodeGen._CONV_TYPE_TO_LLVM_C] += [(type, func)]
    
        return register
    
        
        
codegen = CodeGen()

# We do the void type conversion right here as there is no separate module for it.    
@codegen.convertToLLVM(type.VoidType)
def _(type):
    return llvm.core.Type.void()







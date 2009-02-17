#! /usr/bin/env
#
# LLVM code geneator.
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
        """Resets internal state. After resetting, the object can be used to
        turn another |ast| into LLVM code.
        """
        # Dummy class to create a sub-namespace.
        class _llvm_data:
            pass

        # These state variables are cleared here. Read access must only be via
        # methods of this class. However, some of them are *set* directly by the
        # module/function/block visitors, which avoids adding a ton of setter
        # methods just for these few cases. 
        
        self._module = None        # Current module.
        self._function = None      # Current function.
        self._block = None         # Current block.
        self._func_counter = 0     # Counter to generate unique function names.
        self._label_counter = 0    # Counter to generate unique labels.
        
        self._llvm = _llvm_data()
        self._llvm.module = None   # Current LLVM module.
        self._llvm.functions = {}  # All LLVM functions created so far, indexed by their name.
        self._llvm.frameptr = None # Frame pointer for current LLVM function.
        self._llvm.builders = []   # Stack of llvm.core.Builders

        # Cache some LLVM types.
         
            # The type we use a generic pointer ("void *").
        self._llvm.type_generic_pointer = llvm.core.Type.pointer(llvm.core.Type.int(8)) 

            # A continuation struct.
        self._llvm.type_continuation = llvm.core.Type.struct([
        	self._llvm.type_generic_pointer, # Successor function
            self._llvm.type_generic_pointer  # Frame pointer
            ])

            # Field names with types for the basic frame.
        self._bf_fields = [
        	("cont_normal", self._llvm.type_continuation), 
        	("cont_except", self._llvm.type_continuation),    
		    ("exception", self._llvm.type_generic_pointer),
            ]

            # The basic frame struct. 
        self._llvm.type_basic_frame = llvm.core.Type.struct([ t for (n, t) in self._bf_fields])

    def generateLLVM(self, ast, verify=True):
        """See ~~generateLLVM."""
        self.reset()
        self.visit(ast)
        return self.llvmModule(verify)
            
    def pushBuilder(self, llvm_block):
        """Pushes a LLVM builder on the builder stack. The method creates a
        new ``llvm.core.Builder`` with the given block (which will often be
        just empty at this point), and pushes it on the stack. All ~~Codegen
        methods creating LLVM instructions use the most recently pushed
        builder.
        
        llvm_blockbuilder: llvm.core.Block - The block to create the builder
        with.
        """
        builder = llvm.core.Builder.new(llvm_block) if llvm_block else _DummyBuilder()
        self._llvm.builders += [builder]
        
    def popBuilder(self):
        """Removes the top-most LLVM builder from the builder stack."""
        self._llvm.builders = self._llvm.builders[0:-1]
    
    def builder(self):
        """Returns the top-most LLVM builder from the builder stack. Methods
        creating LLVM instructions should usually use this builder.
        
        Returns: llvm.core.Builder: The most recently pushed builder.
        """
        return self._llvm.builders[-1]

    def currentModule(self):
        """Returns the current module. 
        
        Returns: ~~Module - The current module.
        """
        return self._module
    
    def currentFunction(self):
        """Returns the current function. 
        
        Returns: ~~Function - The current function.
        """
        return self._function
    
    def currentBlock(self):
        """Returns the current block. 
        
        Returns: ~~Block - The current function.
        """
        return self._block
    
    # Looks up the function belonging to the given name. Returns None if not found. 
    def lookupFunction(self, name):
        """Looks up the function with the given name in the current module.
        This is a short-cut for ``currentModule()->LookupID()`` which in
        addition checks (via asserts) that the returns object is indeed a
        function.
        
        Returns: ~~Function - The function with the given name, or None if
        there's no such identifier in the current module's scope.
        """
        func = self._module.lookupID(name)
        assert (not func) or (isinstance(func.type(), type.Function))
        return func
    
    def nameFunction(self, func):
        """Returns the internal LLVM name for the function. The method
        processes the function's name according to HILTI's name mangling
        scheme.
        
        Returns: string - The mangled name, to be used for the corresponding 
        LLVM function.
        """
        name = func.name().replace("::", "_")
        
        if func.callingConvention() == function.CallingConvention.C:
            # Don't mess with the names of C functions.
            return name
        
        if func.callingConvention() == function.CallingConvention.HILTI:
            return "hlt_%s" % name
        
        # Cannot be reached
        assert False
            

    def nameFunctionForBlock(self, block):
    	"""Returns the internal LLVM name for the block's function. The
        compiler turns all blocks into functions, and this method returns the
        name which the LLVM function for the given block should have.
        
        Returns: string - The name of the block's function.
        """
        
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

    def nameNewLabel(self, postfix=None):
        """Returns a unique label for LLVM blocks. The label is guaranteed to
        unique within the :meth:`currentFunction``.
        
        postfix: string - If given, the postfix is appended to the generated label.
        
        Returns: string - The unique label.
        """ 
        self._label_counter += 1
        
        if postfix:
            return "l%d-%s" % (self._label_counter, postfix)
        else:
            return "l%d" % self._label_counter
    
    def nameNewFunction(self, prefix):
        """Returns a unique name for LLVM functions. The name is guaranteed to
        unique within the :meth:`currentModuleFunction``.
        
        prefix: string - If given, the prefix is prepended before the generated name.
        
        Returns: string - The unique name.
        """ 
        self._func_counter += 1
        
        if prefix:
            return "__%s_%s_f%s" % (self._module.name(), prefix, self._func_counter)
        else:
            return "__%s_f%s" % (self._module.name(), self._func_counter)
    
    def llvmNewBlock(self, postfix):
        """Creates a new LLVM block. The block gets a label that is guaranteed
        to be unique within the :meth:`currentFunction``, and it is added to
        the function's implementation.
        
        postfix: string - If given, the postfix is appended to the block's
        label.
        
        Returns: llvm.core.Block - The new block.
        """         
        return self._llvm.func.append_basic_block(self.nameNewLabel(postfix))

    def nameFunctionFrame(self, function):
        """Returns the LLVM name for the struct representing the function's
        frame.
        
        Returns: string - The name of the struct.
        """
        return "__frame_%s" % self.nameFunction(function)
    
    def llvmModule(self, verify=True):
        """Returns the compiled LLVM module. Can only be called once code
        generation has already finished.
        
        *verify*: bool - If true, the correctness of the generated LLVM module
        will be verified via LLVM's internal validator.
        
        Returns: tuple (bool, llvm.core.Module) - If the bool is True, code
        generation (and if *verify* is True, also verification) was
        successful. If so, the second element of the tuple is the resulting
        LLVM module.
        """
        if verify:
            try:
                self._llvm.module.verify()
            except llvm.LLVMException, e:
                util.error("LLVM error: %s" % e, fatal=False)
                return (False, None)
            
        return (True, self._llvm.module)

    def llvmCurrentModule(self):
        """Returns the current LLVM module.
        
        Returns: llvm.core.Module - The LLVM module we're currently building.
        """
        return self._llvm.module
    
    def llvmCurrentFramePtr(self):
        """Returns the frame pointer for the current LLVM function. The frame
        pointer is the base pointer for all accesses to local variables,
        including function arguments). The execution model always passes the
        frame pointer into a function as its first parameter.
        
        Returns: llvm.core.Type.struct - The current frame pointer.
        """
        return self._llvm.frameptr
    
    # Return the LLVM constant representing the given integer.
    def llvmConstInt(self, n, width = 32):
        """Creates an LLVM integer constant.
        
        n: integer - The value of the constant.
        width: interger - The bit-width of the constant.
        
        Returns: llvm.core.Constant.int - The constant.
        """
        return llvm.core.Constant.int(llvm.core.Type.int(width), n)

    # Returns the LLVM constant representing an unset exception.
    def llvmConstNoException(self):
        """Returns the LLVM constant representing an unset exception. This
        constant is written into the basic-frame's exeception field to
        indicate that no exeception has been raised.
        
        Returns: llvm.core.Constant - The constant matching the
        basic-frame's exception field.
        """
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
        ids = function.type().Args() + function.IDs()
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
        
        self._llvm.functions[name] = func

        return func
    
    # Returns LLVM function for the given block, creating one if it doesn't exist yet. 
    def llvmGetFunctionForBlock(self, block):
        name = self.nameFunctionForBlock(block) 
        try:
            return self._llvm.functions[name]
        except KeyError:
            return self.llvmCreateFunctionForBlock(name, block)

    # Returns LLVM for external C function.
    def llvmGetCFunction(self, function):
        name = function.name().replace("::", "_")
        
        try:
            func = self._llvm.functions[name]
            # Make sure it's a C function.
            assert func.calling_convention == llvm.core.CC_C
            return func
        
        except KeyError:
            rt = self.llvmTypeConvert(function.type().resultType())
            args = [self.llvmTypeConvertToC(id.type()) for id in function.type().Args()]
            ft = llvm.core.Type.function(rt, args)
            func = self._llvm.module.add_function(ft, name)
            func.calling_convention = llvm.core.CC_C
            self._llvm.functions[name] = func
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

        if cast_to_type and isinstance(cast_to_type, type.Integer) and \
           isinstance(op.type(), type.Integer) and \
           op.type().width() == 0 and cast_to_type.width() != 0:
               return llvm.core.Constant.int(llvm.core.Type.int(cast_to_type.width()), op.value())

        assert not cast_to_type or cast_to_type == op.type() # Cast not possible
           
        # By now, we can't have any more integers of unspecified size.
        assert not isinstance(op.type(), type.Integer) or op.type().width() > 0

        if isinstance(op.type(), type.Integer):
            return llvm.core.Constant.int(llvm.core.Type.int(op.type().width()), op.value())
        
        if isinstance(op.type(), type.String):
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
@codegen.convertToLLVM(type.Void)
def _(type):
    return llvm.core.Type.void()







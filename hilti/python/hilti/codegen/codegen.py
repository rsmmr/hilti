#! /usr/bin/env
#
# LLVM code geneator.

import sys

import llvm
import llvm.core 

import typeinfo
import system
import inspect

global_id = id

from hilti.core import *
from hilti import instructions

# Class which just absorbs any method calls.
class _DummyBuilder(object):
    def __getattribute__(*args):
        class dummy:
            pass
        return lambda *args: dummy()
    
class CodeGen(visitor.Visitor,objcache.Cache):
    """Implements the generation of LLVM code from an HILTI |ast|."""
    TypeInfo = typeinfo.TypeInfo
    
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
        
        self._libpaths = []        # Search path for libhilti prototypes.
        self._debug = False
        self._module = None        # Current module.
        self._function = None      # Current function.
        self._block = None         # Current block.
        self._func_counter = 0     # Counter to generate unique function names.
        self._label_counter = 0    # Counter to generate unique labels.
        self._glob_counter = 0    # Counter to generate unique constant names.
        
        self._llvm = _llvm_data()
        self._llvm.module = None   # Current LLVM module.
        self._llvm.functions = {}  # All LLVM functions created so far, indexed by their name.
        self._llvm.frameptr = None # Frame pointer for current LLVM function.
        self._llvm.builders = []   # Stack of llvm.core.Builders
        self._llvm.global_ctors = [] # List of registered ctors to run a startup.
        self._llvm.excptypes = {}     # Exception types.

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
		    ("exception", self.llvmTypeExceptionPtr()),
            ]

            # The basic frame struct. 
        self._llvm.type_basic_frame = llvm.core.Type.struct([ t for (n, t) in self._bf_fields])
           
            # The basic type information.
        self._llvm.type_type_information = llvm.core.Type.struct(
                [llvm.core.Type.int(16),            # type
                 llvm.core.Type.int(16),            # sizeof(type)
                 llvm.core.Type.pointer(llvm.core.Type.array(llvm.core.Type.int(8), 0)), # name
                 llvm.core.Type.int(16)]            # num_params
                + [self._llvm.type_generic_pointer] * 3) # functions (cheating a bit with the signature).
                
            # The external function called to report an uncaugt exception. 
        ft = type.Function([], type.Void())
        
    def generateLLVM(self, ast, libpaths, debug=False, verify=True):
        """See ~~generateLLVM."""
        self.reset()
        self._libpaths = libpaths
        self._debug = debug
        self.visit(ast)
        
        # Generate ctors at the very end. 
        self._llvmGenerateCtors()
        
        return self.llvmModule(verify)

    def debugMode(self):
        """Returns whether we are compiling in debugging support. 
        
        Returns: bool - True if compiling with debugging support.
        """
        return self._debug
    
    def pushBuilder(self, llvm_block, builder=None):
        """Pushes a LLVM builder on the builder stack. The method creates a
        new ``llvm.core.Builder`` with the given block (which will often be
        just empty at this point), and pushes it on the stack. All ~~codegen
        methods creating LLVM instructions use the most recently pushed
        builder.
        
        llvm_blockbuilder: llvm.core.Block - The block to create the builder
        with.
        
        builder: ~~llv.core.Builder - Optionally an existing builder can be
        used. Then *llvm_blockbuilder* must be None.
        
        Returns: llvm.core.Builder - The new builder.
        """
        
        if not builder:
            builder = llvm.core.Builder.new(llvm_block) if llvm_block else _DummyBuilder()
        else:
            assert not llvm_blockbuilder
            
        self._llvm.builders += [builder]
        return builder
        
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
    
    def lookupFunction(self, id):
        """Looks up the function for the given ID in the current module.
        This is a short-cut for ``currentModule()->LookupIDVal()`` which in
        addition checks (via asserts) that the returns object is indeed a
        function.
        
        id: string or ~~ID - Either the name of the ID to lookup (see ~~addID
        for the lookup rules used for qualified names), or the ~~ID itself.
        
        Returns: ~~Function - The function with the given name, or None if
        there's no such identifier in the current module's scope.
        """
        func = self._module.lookupIDVal(id)
        assert (not func) or (isinstance(func.type(), type.Function))
        return func
    
    def nameFunction(self, func, prefix=True):
        """Returns the internal LLVM name for the function. The method
        processes the function's name according to HILTI's name mangling
        scheme.
        
        func: ~~Function - The function to generate the name for.
        
        prefix: bool - If false, the internal ``hlt_`` prefix is not prepended.
        
        Returns: string - The mangled name, to be used for the corresponding 
        LLVM function.
        """

        name = func.name()
        
        if func.callingConvention() == function.CallingConvention.C:
            # Don't mess with the names of C functions.
            return name
        
        if func.ID().scope():
            name = func.ID().scope() + "_" + name
        
        if func.callingConvention() == function.CallingConvention.C_HILTI:
            return name
        
        if func.callingConvention() == function.CallingConvention.HILTI:
            return "hlt_%s" % name if prefix else name
            
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

        if name == first_block:
            return funcname
        
        if name.startswith("__"):
            name = name[2:]
    
        return "__%s_%s" % (funcname, name)

    def nameNewLabel(self, postfix=None):
        """Returns a unique label for LLVM blocks. The label is guaranteed to
        be unique within the :meth:`currentFunction``.
        
        postfix: string - If given, the postfix is appended to the generated label.
        
        Returns: string - The unique label.
        """ 
        self._label_counter += 1
        
        if postfix:
            return "l%d-%s" % (self._label_counter, postfix)
        else:
            return "l%d" % self._label_counter
        
    def nameNewGlobal(self, postfix=None):
        """Returns a unique name for global. The name is guaranteed
        to be unique within the :meth:`currentModule``.
        
        postfix: string - If given, the postfix is appended to the generated name.
        
        Returns: string - The unique name.
        """ 
        self._glob_counter += 1
        
        if postfix:
            return "g%d-%s" % (self._glob_counter, postfix)
        else:
            return "g%d" % self._glob_counter
    
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
        
    def nameTypeInfo(self, type):
        """Returns a the name of the global holding the type's meta information.
        
        type: ~~HiltiType - The type. 
        
        Returns: string - The internal name of the global identifier.
        
        Note: It's crucial that this name is the same across all compilation
        units.
        """
        
        canonified = type.name()
        for c in ["<", ">", ",", "{", "}", " ", "*", "="]:
            canonified = canonified.replace(c, "_")
            
        while canonified.find("__") >= 0:
            canonified = canonified.replace("__", "_")
            
        if canonified.endswith("_"):
            canonified = canonified[:-1]
            
        return "hlt_type_info_%s" % canonified

    def llvmTypeTypeInfo(self):
        """Returns the LLVM type for representing a type's meta information.
        The field's of the returned struct correspond to the subset of
        ~~TypeInfo that is provided to the libhilti run-time.
        
        Returns: llvm.core.Type.struct - The type for the meta information.
        
        Note: The struct's layout must match with hlt_type_info as defined #
        in :download:`/libhilti/hilti_intern.h`.
        """
        return self._llvm.type_type_information 

    def getTypeInfo(self, t):
        """Returns the type information for a type.
        
        t: ~~Type - The type to return information for.
        
        Returns: ~~TypeInfo - The type information.
        """
        return self._callCallback(CodeGen._CB_TYPE_INFO, t, [t], must_find=True)
    
    def _createTypeInfo(self, ti):
        th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())
        ti_name = self.nameTypeInfo(ti.type)
        glob = self.llvmCurrentModule().add_global_variable(llvm.core.Type.pointer(th.type), ti_name)
        glob.global_constant = True
        glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY
    
    def _initTypeInfo(self, ti):
        # Creates the type information for a single HiltiType.
        arg_types = []
        arg_vals = []
        
        args = []
        for arg in ti.args:
            # FIXME: We should not hardcode the types here. 
            if isinstance(arg, int):
                at = llvm.core.Type.int(64)
                arg_types += [at]
                arg_vals += [llvm.core.Constant.int(at, arg)]
               
            elif isinstance(arg, type.HiltiType):
                other_ti = self.llvmTypeInfoPtr(arg)
                arg_types += [other_ti.type]
                arg_vals += [other_ti]
                
            else:
                util.internal_error("llvmAddTypeInfos: unsupported type parameter %s" % repr(arg))

        hlt_func_vals = []
        hlt_func_types = []
        
        for conversion in ("to_string", "to_int64", "to_double"):
            if ti.__dict__[conversion]:
                name = ti.__dict__[conversion]
                func = self.currentModule().lookupIDVal(name)
                if not func or not isinstance(func, function.Function):
                    util.internal_error("llvmAddTypeInfos: %s is not a known function" % name)
                    
                func_val = self.llvmGetCFunction(func)
            else:
                func_val = llvm.core.Constant.null(self.llvmTypeGenericPointer())
                
            hlt_func_vals += [func_val]
            hlt_func_types += [func_val.type]

        ti_name = self.nameTypeInfo(ti.type)
        name_name = "%s_name" % ti_name
        
        # Create a global with the type's name.
        name_type = llvm.core.Type.array(llvm.core.Type.int(8), len(ti.name) + 1)
        bytes = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in ti.name + "\0"]
        name_val = llvm.core.Constant.array(llvm.core.Type.int(8), bytes)
        name_glob = self.llvmCurrentModule().add_global_variable(name_type, name_name)
        name_glob.global_constant = True
        name_glob.initializer = name_val
        name_glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY
    
        # Create the type info struct type. Note when changing this we
        # also need to adapt the type in self._llvm.type_type_information
        st = llvm.core.Type.struct(
            [llvm.core.Type.int(16),            # type
             llvm.core.Type.int(16),            # sizeof(type)
             llvm.core.Type.pointer(name_type), # name
             llvm.core.Type.int(16),            # num_params
             self.llvmTypeGenericPointer()]     # aux
            + hlt_func_types + arg_types)

        if ti.type._id == 0:
            util.internal_error("type %s does not have an _id" % ti.type)
        
        aux = ti.aux if ti.aux else llvm.core.Constant.null(self.llvmTypeGenericPointer())
        aux = aux.bitcast(self.llvmTypeGenericPointer())
        
        # Create the type info struct constant.
        sv = llvm.core.Constant.struct(
            [llvm.core.Constant.int(llvm.core.Type.int(16), ti.type._id),  # type
             # Although Constant.sizeof returns an i64, we cast it to an i16
             # because some targets cannot handle 64-bit constant exprs.
             llvm.core.Constant.sizeof(self.llvmTypeConvert(ti.type)).trunc(llvm.core.Type.int(16)),     # sizeof(type)
             name_glob,                                                    # name
             llvm.core.Constant.int(llvm.core.Type.int(16), len(ti.args))] # num_params
            + [aux] + hlt_func_vals + arg_vals)

        glob = self.llvmCurrentModule().get_global_variable_named(ti_name)
        glob.type.refine(llvm.core.Type.pointer(st))
        glob.initializer = sv    
    
    def llvmAddTypeInfos(self):
        """Adds run-time type information to the current LLVM module. The function
        creates one type information object for each instantiated ~~HiltiType.
        
        Note: Currently, we support only integer constants and other
        ~~HiltiType objects as type parameters. 
        """
            
        tis = [self._callCallback(CodeGen._CB_TYPE_INFO, t, [t], must_find=True) for t in type.getAllHiltiTypes()]
        
        for ti in tis:
            if ti:
                self._createTypeInfo(ti)
            
        for ti in tis:
            if ti: 
                self._initTypeInfo(ti)

    def llvmAddTypeInfo(self, t):
        """Adds run-time type information for a type to the current LLVM module. 
        
        t: ~~HiltiType : The type to add type information for.
        """
        assert isinstance(t, type.HiltiType)
        
        ti = self._callCallback(CodeGen._CB_TYPE_INFO, t, [t], must_find=True)
        self._createTypeInfo(ti)
        self._initTypeInfo(ti)
                
    def llvmTypeInfoPtr(self, t):
        """Returns an LLVM pointer to the type information for a type. 
        
        t: ~~Type - The type to return the information for.
        
        Returns: llvm.core.Constant.pointer - The pointer to the type
        information.
        """
        # For decl types, we are interested in the declared type.
        if isinstance(t, type.TypeDeclType):
            t = t.declType()
        
        # We special case ref's here, for which we pass the type info of
        # whatever they are pointing at.
        if isinstance(t, type.Reference):
            t = t.refType()
        
        try:
            ti = self.llvmCurrentModule().get_global_variable_named(self.nameTypeInfo(t))
        except llvm.LLVMException:
            self.llvmAddTypeInfo(t)
            ti = self.llvmCurrentModule().get_global_variable_named(self.nameTypeInfo(t))
            
        assert(ti)
        return ti 
    
    def llvmNewModule(self, name): 
        """Creates a new LLVM module. The module is initialized suitably to
        then be used for translating a HILTi program into.
        
        name: string - The name of the new module.
        
        Returns: ~~llvm.core.Module - The new module.
        """
        prototypes = util.findFileInPaths("libhilti.ll", self._libpaths)
        if not prototypes:
            util.error("cannot find libhilti.ll")
        
        try:
            f = open(prototypes)
        except IOError, e:
            util.error("cannot read %s: %s" % (prototypes, e))
            
        try:
            module = llvm.core.Module.from_assembly(f)
        except llvm.LLVMException, e:
            util.error("cannot parse %s: %s" % (prototypes, e))
        
        module.name = name
        return module
        
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
    
    def llvmConstInt(self, n, width):
        """Creates a LLVM integer constant. The constant is signed. 
        
        n: integer - The value of the constant; it's considered 
        width: interger - The bit-width of the constant.
        
        Returns: llvm.core.Constant.int - The constant, which will be signed.
        """
        return llvm.core.Constant.int(llvm.core.Type.int(width), long(n))
    
    def llvmGEPIdx(self, n):
        """Creates an LLVM integer constant suitable for use as a GEP index.
        
        n: integer - The value of the constant.
        
        Returns: llvm.core.Constant.int - The constant, which will be unsigned.
        """
        return llvm.core.Constant.int(llvm.core.Type.int(32), n)    

    def llvmConstDouble(self, d):
        """Creates an LLVM double constant.
        
        d: double - The value of the constant.
        
        Returns: llvm.core.Constant.int - The constant.
        """
        return llvm.core.Constant.real(llvm.core.Type.double(), d)

    def llvmConstNoException(self):
        """Returns the LLVM constant representing an unset exception. This
        constant is written into the basic-frame's exeception field to
        indicate that no exeception has been raised.
        
        Returns: llvm.core.Constant - The constant matching the
        basic-frame's exception field.
        """
        return llvm.core.Constant.null(self.llvmTypeExceptionPtr())

    def llvmTypeExceptionPtr(self):
        """Returns the LLVM type representing an exception.
        
        Returns: llvm.core.Type - The type of an exception.
        """
        return self.llvmTypeGenericPointer()

    def llvmTypeExceptionType(self):
        """Returns the LLVM type representing the *type* of an exception. Also
        see ``libhilti/exceptions.h``.
        
        Returns: llvm.core.Type - The type of an exception type.
        """
        
        th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())
        st = llvm.core.Type.struct(
            [llvm.core.Type.pointer(llvm.core.Type.int(8)), # Name
            llvm.core.Type.pointer(th.type),                # Parent
            self.llvmTypeGenericPointer()])                 # Arg type.
        
        th.type.refine(st)
          
        return  th.type

    def llvmAddExceptionType(self, etype):
        """Adds a new exception type to the current LLVM module. If the
        exception type has already been added before, the existing one is
        returned. 
        
        etype: ~~type.Exception - The type of the exception. 
        
        Returns: llvm.core.Value - The LLVM value representing the new type
        object.
        """
        
        if etype.isRootType():
            # Special-case: the root type is defined externally.
            return self.llvmGetGlobalConst("hlt_exception_unspecified", self.llvmTypeExceptionType(), ptr=True)
        
        name = etype.exceptionName()
        
        if name in self._llvm.excptypes:
            return self._llvm.excptypes[name]

        self._llvm.excptypes[name] = None # Avoid infinite recursion.
        
        assert etype.baseClass()
        
        lname = self.llvmAddGlobalStringConst(etype.exceptionName(), "excpt-name")
        if etype.argType():
            tinfo = self.llvmTypeInfoPtr(etype.argType()).bitcast(self.llvmTypeGenericPointer())
        else:
            tinfo = llvm.core.Constant.null(self.llvmTypeGenericPointer())
        baseval = self.llvmAddExceptionType(etype.baseClass())
        
        assert baseval
        
        val = llvm.core.Constant.struct([lname, baseval, tinfo])
        glob = self.llvmAddGlobalConst(val, "exception-type")
        
        self._llvm.excptypes[name] = glob
        return glob
    
    def llvmTypeGenericPointer(self):
        """Returns the LLVM type that we use for representing a generic
        pointer. (\"``void *``\").
        
        Returns: llvm.core.Type.pointer - The pointer type.
        """
        return self._llvm.type_generic_pointer

    def llvmTypeFunctionPtr(self, function):
        """Returns the LLVM type for representing a pointer to the function.
        The type is built according to the function's signature and the
        internal LLVM execution model.
        
        Returns: llvm.core.Type.pointer - The function pointer type.
        """
        voidt = llvm.core.Type.void()
        ft = llvm.core.Type.function(voidt, [llvm.core.Type.pointer(self.llvmTypeFunctionFrame(function))])
        return llvm.core.Type.pointer(ft)

    def llvmTypeBasicFunctionPtr(self, args=[]):
        """Returns the LLVM type for representing a pointer to an LLVM
        function taking a basic-frame as its argument. Additional additional
        can be optionally given as well. The function type will have ``void``
        as it's return type.
        
        args: list of ~~Type - Additional arguments for the function, which
        will be converted into the corresponding LLVM types.
        
        Returns: llvm.core.Type.pointer - The function pointer type. 
        """
        voidt = llvm.core.Type.void()
        args = [self.llvmTypeConvert(t) for t in args]
        bf = [llvm.core.Type.pointer(self.llvmTypeBasicFrame())]
        ft = llvm.core.Type.function(voidt, bf + args)
        return llvm.core.Type.pointer(ft)
    
    def llvmTypeContinuation(self):
        """Returns the LLVM type used for representing continuations.
        
        Returns: llvm.core.Type.struct - The type for a continuation.
        """
        return self._llvm.type_continuation
    
    def llvmContinuation(self, succ, frame):
        """Create a new continuation object. The method uses the current
        :meth:`builder`.

        succ: llvm.core.Function - The entry point for the continuation
        frame: llvm.core.Value - The frame pointer for the continuation.
        
        Returns: llvm.core.Value - The continuation object. 
        """
        cont = self.llvmMalloc(self.llvmTypeContinuation())
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1) 

        succ_addr = self.builder().gep(cont, [zero, zero])
        frame_addr = self.builder().gep(cont, [zero, one])

        succ = self.builder().bitcast(succ, self.llvmTypeGenericPointer())
        frame = self.builder().bitcast(frame, self.llvmTypeGenericPointer())
        
        self.llvmInit(succ, succ_addr)
        self.llvmInit(frame, frame_addr)
        
        return cont

    def llvmResumeContinuation(self, cont):
        """Resume a continuation object.

        cont: llvm.core.Value - The continuation object. 
        
        Todo: This function has not been tested yet. 
        """
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1) 

        succ_addr = self.builder().gep(cont, [zero, zero])
        frame_addr = self.builder().gep(cont, [zero, one])

        succ_addr = self.builder().bitcast(succ_addr, llvm.core.Type.pointer(self.llvmTypeBasicFunctionPtr()))
        frame_addr = self.builder().bitcast(frame_addr, llvm.core.Type.pointer(llvm.core.Type.pointer(self.llvmTypeBasicFrame())))
        
        succ = self.builder().load(succ_addr)
        frame = self.builder().load(frame_addr)

        # Clear exception information.
        addr = self.llvmAddrException(frame)
        zero = self.llvmConstNoException()
        self.llvmInit(zero, addr)

        self.llvmGenerateTailCallToFunctionPtr(succ, [frame])
    
    def llvmTypeBasicFrame(self):
        """Returns the LLVM type used for representing a basic-frame.
        
        Returns: llvm.core.Type.struct - The type for a basic-frame.
        """
        return self._llvm.type_basic_frame

    def llvmTypeFunctionFrame(self, function):
        """Returns the LLVM type used for representing the function's frame.
        
        function: ~~Function - The function.
        
        Returns: llvm.core.Type.struct - The type for the function's frame.
        """
        
        try:
            # Is it cached?
            if function._llvm_frame != None:
                return function._llvm_frame
        except AttributeError:
            pass
        
        locals = [i for i in function.IDs() if isinstance(i.type(), type.ValueType)]
        fields = self._bf_fields + [(i.name(), self.llvmTypeConvert(i.type())) for i in locals]

        # Build map of indices and store with function.
        function._frame_index = {}
        for i in range(0, len(fields)):
            function._frame_index[fields[i][0]] = i

        frame = [f[1] for f in fields]
        function._llvm_frame = llvm.core.Type.struct(frame) # Cache it.
        
        return function._llvm_frame

    def llvmAllocFunctionFrame(self, func):
        """Allocates a new frame for calling a function. The local variables
        will be initialized to their default values, all other frame fields
        are not initialized. 
        
        func: ~~Function - The function to allocate the frame for.
        
        Returns: llvm.core.Value - The address of the allocated frame.
        """
        frame = self.llvmMalloc(self.llvmTypeFunctionFrame(func))

        for i in func.IDs():
            
            if i.type() == type.Label:
                continue
            
            assert isinstance(i.type(), type.ValueType)
            addr = self.llvmAddrLocalVar(func, frame, i.name())
            self.llvmInitWithDefault(i.type(), addr)
        
        return frame

    def llvmAlloca(self, type, tag=""):
        """Allocates an LLVM variable on the stack. The variable can be used
        only inside the current LLVM function. That function must already have
        at least one block.
        
        type: llvm.core.Type - The tye of the variable to allocate.
        
        Note: This works exactly like LLVM's ``alloca`` instruction
        yet it makes sure that the allocs are located in the current
        function entry block. That is a requirement for LLVM's
        ``mem2reg`` optimization pass to find them."""
        
        b = self._llvm.func.get_entry_basic_block()
        builder = llvm.core.Builder.new(b)
        
        if b.instructions:
            # FIXME: This crashes when the block is empty. Is there a more
            # efficeint way to detection whether it's empty than getting all
            # instruction? Or better, can we fix llvm-py? Update: this is fixed
            # in llvm-py SVN. Change this once we update.
            builder.position_at_beginning(b)
            
        return self.builder().alloca(type)
     
    # Helpers to get a field in a basic frame struct.
    def _llvmAddrInBasicFrame(self, frame, index1, index2, cast_to, tag):
        zero = self.llvmGEPIdx(0)
        index1 = self.llvmGEPIdx(index1)
        
        if index2 >= 0:
            index2 = self.llvmGEPIdx(index2)
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
    def llvmAddrDefContSuccessor(self, frameptr, cast_to = None):
        """Returns the address of the default continuation's successor field
        inside a function's frame. The method uses the current :meth:`builder`
        to calculate the address and returns an LLVM value object that can be
        directly used with subsequent LLVM load or store instructions.
        
        frameptr: llvm.core.* - The address of the function's frame. This
        must be an LLVM object that can act as the frame's address in the LLVM
        instructions that will be built to calculate the field's address.
        Examples include corresponding ``llvm.core.Argument`` and
        ``llvm.core.Instruction`` objects. 
        
        cast_to: llvm.core.Type.pointer - If given, the address will be
        bit-casted to the given pointer type.
        
        Returns: llvm.core.Value - An LLVM value with the address. If
        *cast_to* was not given, its type will be a pointer to a
        :meth:`llvmTypeGenericPointer`. 
        """
        return self._llvmAddrInBasicFrame(frameptr, 0, 0, cast_to, "cont_succ")
    
    # Returns the address of the default continuation's frame field in the given frame.
    def llvmAddrDefContFrame(self, frameptr, cast_to = None):
        """Returns the address of the default continuation's frame field
        inside a function's frame. The method uses the current :meth:`builder`
        to calculate the address and returns an LLVM value object that can be
        directly used with subsequent LLVM load or store instructions.
        
        frameptr: llvm.core.* - The address of the function's frame. This
        must be an LLVM object that can act as the frame's address in the LLVM
        instructions that will be built to calculate the field's address.
        Examples include corresponding ``llvm.core.Argument`` and
        ``llvm.core.Instruction`` objects. 
        
        cast_to: llvm.core.Type.pointer - If given, the address will be
        bit-casted to the given pointer type.
        
        Returns: llvm.core.Value - An LLVM value with the address. If
        *cast_to* was not given, its type will be a pointer to a
        :meth:`llvmTypeGenericPointer`. 
        """
        return self._llvmAddrInBasicFrame(frameptr, 0, 1, cast_to, "cont_frame")

    # Returns the address of the exception continuation's successor field in the given frame.
    def llvmAddrExceptContSuccessor(self, frameptr, cast_to = None):
        """Returns the address of the exceptional continuation's successor
        field inside a function's frame. The method uses the current
        :meth:`builder` to calculate the address and returns an LLVM value
        object that can be directly used with subsequent LLVM load or store
        instructions.
        
        frameptr: llvm.core.* - The address of the function's frame. This
        must be an LLVM object that can act as the frame's address in the LLVM
        instructions that will be built to calculate the field's address.
        Examples include corresponding ``llvm.core.Argument`` and
        ``llvm.core.Instruction`` objects. 
        
        cast_to: llvm.core.Type.pointer - If given, the address will be
        bit-casted to the given pointer type.
        
        Returns: llvm.core.Value - An LLVM value with the address. If
        *cast_to* was not given, its type will be a pointer to a
        :meth:`llvmTypeGenericPointer`. 
        """
        return self._llvmAddrInBasicFrame(frameptr, 1, 0, cast_to, "excep_succ")
    
    # Returns the address of the exception continuation's frame field in the given frame.
    def llvmAddrExceptContFrame(self, frameptr, cast_to = None):
        """Returns the address of the exceptional continuation's frame field
        inside a function's frame. The method uses the current :meth:`builder`
        to calculate the address and returns an LLVM value object that can be
        directly used with subsequent LLVM load or store instructions.
        
        frameptr: llvm.core.* - The address of the function's frame. This must be an
        LLVM object that can act as the frame's address in the LLVM
        instructions that will be built to calculate the field's address.
        Examples include corresponding ``llvm.core.Argument`` and
        ``llvm.core.Instruction`` objects. 
        
        cast_to: llvm.core.Type.pointer - If given, the address will be
        bit-casted to the given pointer type.
        
        Returns: llvm.core.Value - An LLVM value with the address. If
        *cast_to* was not given, its type will be a pointer to a
        :meth:`llvmTypeGenericPointer`. 
        """
        return self._llvmAddrInBasicFrame(frameptr, 1, 1, cast_to, "excep_frame")

    # Returns the address of the current exception field in the given frame.
    def llvmAddrException(self, frameptr):
        """Returns the address of the exception field inside a function's
        frame. The method uses the current :meth:`builder` to calculate the
        address and returns an LLVM value object that can be directly used
        with subsequent LLVM load or store instructions.
        
        frameptr: llvm.core.* - The address of the function's frame. This
        must be an LLVM object that can act as the frame's address in the LLVM
        instructions that will be built to calculate the field's address.
        Examples include corresponding ``llvm.core.Argument`` and
        ``llvm.core.Instruction`` objects. 
        
        cast_to: llvm.core.Type.pointer - If given, the address will be
        bit-casted to the given pointer type.
        
        Returns: llvm.core.Value - An LLVM value with the address. If
        *cast_to* was not given, its type will be a pointer to a
        :meth:`llvmTypeExceptionPtr`. 
        """
        return self._llvmAddrInBasicFrame(frameptr, 2, -1, None, "excep_frame")
    
    def llvmAddrLocalVar(self, func, frameptr, name):
        """Returns the address of a local variable inside a function's frame.
        The method uses the current :meth:`builder` to calculate the address
        and returns an LLVM value object that can be directly used with
        subsequent LLVM load or store instructions.
        
        func: ~~Function - The function whose local variable should be
        accessed.
        
        frameptr: llvm.core.* or None - The address of the function's frame. This must
        be an LLVM object that can act as the frame's address in the LLVM
        instructions that will be built to calculate the field's address.
        Often, it will be the result of :meth:`llvmCurrentFramePtr`. The frame
        can be None if the function is not of ~~Linkage ~~HILTI.
        
        name: string - The name of the local variable. 

        Returns: llvm.core.Value - An LLVM value with the address. 
        """

        if func.callingConvention() == function.CallingConvention.HILTI:
            try:
                # See whether we already calculated the field index dict. 
                frame_idx = func._frame_index
            except AttributeError:
                # Not yet calculated.
                self.llvmTypeFunctionFrame(func)
                frame_idx = func._frame_index
                
            # The Python interface (as well as LLVM's C interface, which is used
            # by the Python interface) does not have builder.extract_value() method,
            # so we simulate it with a gep/load combination.
            zero = self.llvmGEPIdx(0)
            index = self.llvmGEPIdx(frame_idx[name])
            return self.builder().gep(frameptr, [zero, index], name)
        
        else:
            # We create the locals dynamically on the stack for C funcs.
            try:
                clocals = func._clocals
            except AttributeError:
                clocals = {}
                func._clocals = clocals

            if name in clocals:
                return clocals[name]
                
            for i in func.IDs():
                if i.name() == name:
                    addr = self.llvmAlloca(self.llvmTypeConvert(i.type()), tag=name)
                    clocals[name] = addr
                    return addr

            util.internal_error("unknown local")

    def llvmAddrGlobalVar(self, i):
        """Returns the address of a global variable inside the current module.
        
        i: ~~ID - The ID corresponding the global.

        Returns: llvm.core.Value - An LLVM value with the address. 
        """
        glob = self._llvm.module.get_global_variable_named(i.name())
        
        if isinstance(i.type(), type.Reference):
            # Need an additional dereference operation.
            glob = self.builder().load(glob, "deref")
            
        return glob
            
    def llvmGetGlobalVar(self, name, type, ptr=False):
        """Generates an LLVM load instruction for reading a global variable.
        The global can be either defined in the current module, or externally
        (the latter is assumed if we don't fine the variable locally). The
        load is built via the current :meth:`builder`.
        
        name: string - The internal LLVM name of the global. 
        
        type: llvm.core.Type - The LLVM type of the global.
        
        ptr: bool - If true, the returned value represents a pointer to the
        global, rather than the global's value.
        
        Returns: llvm.core.Value - The result of the LLVM load instruction. 
        """
        try:
            glob = self._llvm.module.get_global_variable_named(name)
            # FIXME: Check that the type is the same.
            return self.builder().load(glob, name) if not ptr else glob
        except llvm.LLVMException:
            pass

        assert type
        glob = self._llvm.module.add_global_variable(type, name)
        glob.linkage = llvm.core.LINKAGE_EXTERNAL 
        return self.builder().load(glob, name) if not ptr else glob

    def llvmGetGlobalConst(self, name, type, ptr=False):
        """Generates an LLVM load instruction for reading a global constant.
        The global can be either defined in the current module, or externally
        (the latter is assumed if we don't fine the variable locally). The
        load is built via the current :meth:`builder`.
        
        name: string - The internal LLVM name of the global. 
        
        type: llvm.core.Type - The LLVM type of the global.
        
        ptr: bool - If true, the returned value represents a pointer to the
        global, rather than the global's value.
        
        Returns: llvm.core.Value - The result of the LLVM load instruction. 
        """
        glob = self.llvmGetGlobalVar(name, type, ptr)
        glob.global_constant = True
        return glob
    
    # Creates a new func matching the block's frame/result. Additional
    # arguments to the function can be specified by passing (name, type)
    # tuples via addl_args.
    def llvmCreateFunction(self, hilti_func, name = None, addl_args = []):
        """Creates a new LLVM function. The new function's signatures is built
        from the given HILTI function, according to the HILTI calling
        conventions. The function is added to :meth:`llvmCurrentModule`.
        
        hilti_func: ~~Function - The HILTI function determining the signature. 
        
        name: string - If given, the name of the new function. If not given,
        the result of :meth:`nameFunction` is used. 
        
        addl_args: list of (name, type) - Additional arguments to add to new
        the function's argument list. *name* is a ``string`` with the argument's
        name, and *type* is a ``llvm.core.Type`` with the argument's type.
        
        Returns: llvm.core.Function - The new LLVM function. 
        """

        if not name:
            name = self.nameFunction(hilti_func)
        
        # Build function type.
        args = [llvm.core.Type.pointer(self.llvmTypeFunctionFrame(hilti_func))]
        ft = llvm.core.Type.function(llvm.core.Type.void(), args + [t for (n, t) in addl_args]) 
        
        # Create function.
        func = self._llvm.module.add_function(ft, name)
        func.calling_convention = llvm.core.CC_FASTCALL
        
        if hilti_func.linkage() != function.Linkage.EXPORTED:
            func.linkage = llvm.core.LINKAGE_INTERNAL
        
        # Set parameter names. 
        func.args[0].name = "__frame"
        for i in range(len(addl_args)):
            func.args[i+1].name = addl_args[i][0]

        self._llvm.functions[name] = func

        return func

    def llvmGetFunction(self, func):
        """Returns the LLVM function for the given function. If the LLVM
        function doesn't exist yet, it will be created. This method can only be
        used for functions with ~~Linkage ~~HILTI.
        
        func: ~~Function - The function. 
        
        Returns: llvm.core.Function - The LLVM function.
        """
        assert func.callingConvention() == function.CallingConvention.HILTI
        
        try:
            name = self.nameFunction(func)
            return self._llvm.functions[name]
        except KeyError:        
            return self.llvmCreateFunction(func)
    
    def llvmGetFunctionForBlock(self, block):
        """Returns the LLVM function for a block. If the LLVM function doesn't
        exist yet, it will be created.
        
        block: ~~Block - The block for which the LLVM function should be
        returned.
        
        Returns: llvm.core.Function - The LLVM function for the block.
        """
        name = self.nameFunctionForBlock(block) 
        try:
            return self._llvm.functions[name]
        except KeyError:
            return self.llvmCreateFunction(block.function(), name)

    def _llvmMakeArgTypesForCHiltiCall(self, func, arg_types):
        # Turn the argument types into the format used by CC C_HILTI (see there). 
        new_arg_types = []
        for (argty, prototype) in zip(arg_types, func.type().args()):
            p = prototype.type()
            if isinstance(p, type.Any) or \
               (isinstance(p, type.HiltiType) and p.wildcardType()):
                # Add a type information parameter.
                new_arg_types += [llvm.core.Type.pointer(self.llvmTypeTypeInfo())]
                # Turn argument into a pointer. 
                new_arg_types += [self.llvmTypeGenericPointer()]
            else:
                new_arg_types += [argty]
            
        # Add exception argument.
        new_arg_types += [llvm.core.Type.pointer(self.llvmTypeGenericPointer())]
        return new_arg_types
    
    def _llvmMakeRetTypeForCHiltiCall(self, func, rt):
        # Turn the return type into the format used by CC C_HILTI. If the return
        # type equals to ``any`` it is simply converted to a void* pointer.
        if isinstance(func.type().resultType(), type.Any):
            return self.llvmTypeGenericPointer()
        
        return rt
        
    def _llvmMakeArgsForCHiltiCall(self, func, args, arg_types, excpt_ptr):
        # Turns the arguments into the format used by CC C_HILTI (see there). 
        new_args = []
        for (arg, argty, prototype) in zip(args, arg_types, func.type().args()):
            p = prototype.type()
            if isinstance(p, type.Any) or \
               (isinstance(p, type.HiltiType) and p.wildcardType()):
                # Add a type information parameter.
                new_args += [self.builder().bitcast(self.llvmTypeInfoPtr(argty), llvm.core.Type.pointer(self.llvmTypeTypeInfo()))]
                # Turn argument into a pointer.
                ptr = self.llvmAlloca(arg.type)
                self.builder().store(arg, ptr)
                ptr = self.builder().bitcast(ptr, self.llvmTypeGenericPointer())
                new_args += [ptr]
            elif isinstance(p, type.MetaType):
                new_args += [self.builder().bitcast(self.llvmTypeInfoPtr(argty), llvm.core.Type.pointer(self.llvmTypeTypeInfo()))]
            else:
                new_args += [arg]
            
        # Add exception argument.
        if excpt_ptr:
            new_args += [excpt_ptr]
            
        return new_args
        
    def llvmGetCFunction(self, func):
        """Returns the LLVM function for an external C function. If the LLVM
        function doesn't exist yet, it is be created.
        
        func: ~~Function - The function which must be of ~~CallingConvention
        ~~C or ~~C_HILTI.
        
        Returns: llvm.core.Function - The LLVM function for the external C
        function.
        """
                
        assert func.callingConvention() in (function.CallingConvention.C, function.CallingConvention.C_HILTI)
        
        name = self.nameFunction(func)
        
        try:
            func = self._llvm.functions[name]
            # Make sure it's a C function.
            assert func.calling_convention == llvm.core.CC_C
            return func
        
        except KeyError:
            rt = self.llvmTypeConvert(func.type().resultType())
            if func.callingConvention() == function.CallingConvention.C_HILTI:
                rt = self._llvmMakeRetTypeForCHiltiCall(func, rt)

            args = [self.llvmTypeConvert(id.type()) for id in func.type().args()]
            
            if func.callingConvention() == function.CallingConvention.C_HILTI:
                args = self._llvmMakeArgTypesForCHiltiCall(func, args)

            # Depending on the plaform ABI, structs may or may not be returned
            # per value. values. If not, there's a hidden first parameter
            # passing a pointer into the function to store the value in, instead
            # of the actual return value.
            hidden_return_arg = None
            cast_return = None
            
            if isinstance(rt, llvm.core.StructType):
                orig_rt = rt
                rt = system.returnStructByValue(rt)
                if not rt:
                    hidden_return_arg = orig_rt
                    args = [llvm.core.Type.pointer(orig_rt)] + args
                    rt = llvm.core.Type.void()
                else:
                    hidden_return_arg = None
                    cast_return = orig_rt
                
            ft = llvm.core.Type.function(rt, args)
            llvmfunc = self._llvm.module.add_function(ft, name)
            llvmfunc.calling_convention = llvm.core.CC_C
            llvmfunc.hidden_return_arg = hidden_return_arg
            llvmfunc.cast_return = cast_return
            
            self._llvm.functions[name] = llvmfunc
            return llvmfunc

    def _llvmGenerateExceptionTest(self, excpt_ptr, abort_on_except=False):
        # Generates code to check whether a called C-HILTI function has raised
        # an exception.
        excpt = self.builder().load(excpt_ptr)
        raised = self.builder().icmp(llvm.core.IPRED_NE, excpt, self.llvmConstNoException())

    	block_excpt = self.llvmNewBlock("excpt")
        block_noexcpt= self.llvmNewBlock("noexcpt")
        self.builder().cbranch(raised, block_excpt, block_noexcpt)
    
        self.pushBuilder(block_excpt)
        
        if not abort_on_except and self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            self.llvmRaiseException(excpt)
        else:
            abort = codegen.llvmCurrentModule().get_function_named("__hlt_exception_print_uncaught_abort")
            self.builder().call(abort, [excpt])
            self.builder().unreachable()

        self.popBuilder()
    
        # We leave this builder for subseqent code.
        self.pushBuilder(block_noexcpt)
    
    def llvmGenerateCCall(self, func, args, arg_types=None, llvm_args=False, abort_on_except=False):
        """Generates a call to a C function. The method uses the current
        :meth:`builder`.
        
        func: ~~Function - The function, which must have ~~CallingConvention ~~C or
        ~~C_HILTI. 
        
        args: list of ~~Operand, or list of llvm.core.Value - The arguments to
        pass to the function. If they are already converted to LLVM, then
        *llvm_args* must be True.
        
        arg_types: list ~~Type - The types of the actual arguments.
        While these will often be the same as specified in the
        function's signature, they can differ in some cases, e.g.,
        if the function accepts an argument of type ~~Any. If missing, the
        types are derived from *args* (which then must be ~~Operand).
        
        llvm_args: boolean - If true, *args* are expected to be of type
        ``llvm.core.Value``.

        abort_on_except: boolean - If true, the generated code will *abort*
        execution if an exception is thrown by the called function. In this
        case, the code does not need access to a HILTI function's frame and be
        used inside functions that don't follow normal calling conventions (in
        particular in code generated via ~~llvmAddGlobalCtor).
        
        Returns: llvm.core.Value - A value representing the function's return
        value. 
        """
        assert func.callingConvention() == function.CallingConvention.C or func.callingConvention() == function.CallingConvention.C_HILTI
        
        llvm_func = self.llvmGetCFunction(func)

        if not llvm_args:
            if not arg_types:
                arg_types = [op.type() for op in args]
            
            args = [self.llvmOp(op) for op in args]
        else:
            assert arg_types
        
        if func.callingConvention() != function.CallingConvention.C:
            if not abort_on_except and self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
                excpt_ptr = self.llvmAddrException(self.llvmCurrentFramePtr())
            else:
                # Create a local to store the exception information so that we don't
                # need a function frame. 
                excpt_ptr = codegen.llvmAlloca(codegen.llvmTypeExceptionPtr())
                excpt_ptr = codegen.builder().bitcast(excpt_ptr, llvm.core.Type.pointer(codegen.llvmTypeGenericPointer()))
                zero = self.llvmConstNoException()
                self.llvmInit(zero, excpt_ptr)
        else:
            # No exception test necessary (nor possible).
            excpt_ptr = None
            
        if func.callingConvention() == function.CallingConvention.C_HILTI:
            args = self._llvmMakeArgsForCHiltiCall(func, args, arg_types, excpt_ptr)
            
        if llvm_func.hidden_return_arg:
            hidden_return_arg = self.llvmAlloca(llvm_func.hidden_return_arg, "agg.tmp")
            args = [hidden_return_arg] + args            
            call = self.builder().call(llvm_func, args)
            call.add_parameter_attribute(1, llvm.core.ATTR_NO_ALIAS)
            call.add_parameter_attribute(1, llvm.core.ATTR_STRUCT_RET)
            # call.add_parameter_attribute(0, llvm.core.ATTR_NO_CAPTURE) # FIXME: llvm-py doesn't have this.
            call = self.builder().load(hidden_return_arg)
            result = call
            
        elif func.type().resultType() == type.Void:
            call = self.builder().call(llvm_func, args)
            result = call
            
        else:
            call = self.builder().call(llvm_func, args, "result")
            result = call            
            
            if llvm_func.cast_return:
                # We can't cast an integer into a struct. Let's hope the
                # optimizer is able to remove this tmp.
                #
                # FIXME: Turns out, currently it is not. What can we do about
                # that?
                tmp = self.llvmAlloca(llvm_func.cast_return)
                tmp_casted = self.builder().bitcast(tmp, llvm.core.Type.pointer(call.type))
                self.builder().store(call, tmp_casted)
                result = self.builder().load(tmp)
                
        call.calling_convention = llvm.core.CC_C

        if excpt_ptr:
            self._llvmGenerateExceptionTest(excpt_ptr, abort_on_except)
            
        return result

    def llvmGenerateCCallByName(self, name, args, arg_types = None, llvm_args=False, abort_on_except=False):
        """Generates a call to a C function given by it's name. The method
        uses the current :meth:`builder`.
        
        name: string - The HILTI-level name of the C function, including the
        function's module namespace (e.g., ``Hilti::print`` or
        ``hlt::string_len``). A function with this name must have been
        declared inside the current module already; the method will report an
        internal error if it hasn't.
        
        args: list of ~~Operand, or list of llvm.core.Value - The arguments to
        pass to the function. If they are already converted to LLVM, then
        *llvm_args* must be True.
        
        arg_types: list ~~Type - The types of the actual arguments.
        While these will often be the same as specified in the
        function's signature, they can differ in some cases, e.g.,
        if the function accepts an argument of type ~~Any. If missing, the
        types are derived from *args* (which then must be ~~Operand).

        llvm_args: boolean - If true, *args* are expected to be of type
        ``llvm.core.Value``.
        
        abort_on_except: boolean - If true, the generated code will *abort*
        execution if an exception is thrown by the called function. In this
        case, the code does not need access to a HILTI function's frame and be
        used inside functions that don't follow normal calling conventions (in
        particular in code generated via ~~llvmAddGlobalCtor).
        
        Returns: llvm.core.Value - A value representing the function's return
        value. 
        
        Note: This method is just a short-cut for calling ~~llvmGenerateCCall
        with the right ~~Function object. 
        """
        func = self.currentModule().lookupIDVal(name)
        if not func or not isinstance(func, function.Function):
            util.internal_error("llvmGenerateCCallByName: %s is not a known function" % name)
            
        return self.llvmGenerateCCall(func, args, arg_types, llvm_args, abort_on_except)

    def llvmCStubs(self, func):
        """Returns the C stubs for a function. The stub functions will be 
        externally visible functions using standard C calling conventions, and
        expecting parameters corresponding to the function's HILTI parameters,
        per the usual C<->HILTI translation. The primary stub function gets
        one additional parameter of type ``hlt_exception *``, which the callee
        can set to an exception object to signal that an exception was thrown.
        The secondary function is for resuming after a previous ~~Yield; 
        it gets two parameters, the ~~YieldException and a ``hlt_exception *``
        with the same semantics as for the primary function.
        
        All the primary stub does when called is in turn calling the HILTI
        function, adapting the calling conventions. 
                
        func: ~~Function - The function to generate a stub for. The function
        must have ~~HILTI calling convention.
        
        Returns: 2-tuple of llvm.core.Value.Function - The two functions. 
        
        Note: ``hiltic`` can generate the right prototypes for such stub
        functions.
        """
        
        def _makeStubs():
            return self._llvmGenerateCStubs(func)
        
        return self.cache("c-stub-%s" % func.name(), _makeStubs )
    
    def _llvmGenerateCStubs(self, func):
        """Internal function to generate the stubs, without caching.
        
        Todo: This function needs a cleanup. There's a lot of code duplication
        between the primary function and the resume function.
        """
        assert func.callingConvention() == function.CallingConvention.HILTI

        is_void = (func.type().resultType() == type.Void)
        name = self.nameFunction(func, prefix=False)
        
        # Create the stub function.
        if not is_void:
            result_type = self.llvmTypeConvert(func.type().resultType())
            addl_arg = [result_type]
        else:
            result_type = llvm.core.Type.void()
            addl_arg = []
        
        rt = result_type
        args = [self.llvmTypeConvert(id.type()) for id in func.type().args()]
        args += [llvm.core.Type.pointer(self.llvmTypeExceptionPtr())]
        ft = llvm.core.Type.function(rt, args)
        llvm_func = self._llvm.module.add_function(ft, name)
        llvm_func.calling_convention = llvm.core.CC_C
        self._llvm.functions[name] = llvm_func
        block = llvm_func.append_basic_block("")
        self.pushBuilder(block)

        # Create a frame for us. We add one variable to store the return
        # value if we have one.
        if not is_void:
            fields = self._bf_fields + [("result", self.llvmTypeConvert(func.type().resultType()))]
            frame_type = llvm.core.Type.struct([f[1] for f in fields])
            stub_frame = self.llvmMalloc(frame_type)
        else:
            stub_frame = self.llvmMalloc(self.llvmTypeBasicFrame())
        
        # Create the return function.
        return_name = name + "_return"
        args = [stub_frame.type] + addl_arg
        rt = llvm.core.Type.void()
    	ft = llvm.core.Type.function(rt, args)  
        return_func = self._llvm.module.add_function(ft, return_name)
        return_func.calling_convention = llvm.core.CC_FASTCALL
        return_func.linkage = llvm.core.LINKAGE_INTERNAL
        return_func.args[0].name = "__frame"
        if addl_arg:
	        return_func.args[1].name = "result"
            
        return_block = return_func.append_basic_block("")
        self.pushBuilder(return_block)
        
        if not is_void:
            # Store it in frame. 
            zero = self.llvmGEPIdx(0)
            index = self.llvmGEPIdx(len(self._bf_fields))
            addr = self.builder().gep(return_func.args[0], [zero, index])
            self.builder().store(return_func.args[1], addr)
            
        self.builder().ret_void()
        self.popBuilder()

        # Create the exception handler.
        excpt_name = name + "_excpt"
    	ft = llvm.core.Type.function(llvm.core.Type.void(), [stub_frame.type])  
        excpt_func = self._llvm.module.add_function(ft, excpt_name)
        excpt_func.calling_convention = llvm.core.CC_FASTCALL
        excpt_func.linkage = llvm.core.LINKAGE_INTERNAL
        excpt_func.args[0].name = "__frame"
        excpt_block = excpt_func.append_basic_block("")
        self.pushBuilder(excpt_block)
            # Nothing to do in the handler.
        self.builder().ret_void()
        self.popBuilder()
        
        # Create the call.
        
            # Allocate new stack frame for called function.
        callee_frame = self.llvmAllocFunctionFrame(func)
   
            # After call, continue with our return function and give it our
            # frame.
        addr = self.llvmAddrDefContSuccessor(callee_frame, llvm.core.Type.pointer(return_func.type))
        self.llvmInit(return_func, addr)
        addr = self.llvmAddrDefContFrame(callee_frame, llvm.core.Type.pointer(stub_frame.type))
        self.llvmInit(stub_frame, addr)

            # Set the exception handler and also give it our frame.
        addr = self.llvmAddrExceptContSuccessor(callee_frame, llvm.core.Type.pointer(excpt_func.type))
        self.llvmInit(excpt_func, addr)
        addr = self.llvmAddrExceptContFrame(callee_frame, llvm.core.Type.pointer(stub_frame.type))
        self.llvmInit(stub_frame, addr)
        
            # Clear exception information.
        addr = self.llvmAddrException(callee_frame)
        zero = self.llvmConstNoException()
        self.llvmInit(zero, addr)
        
            # Initialize function arguments. 
        ids = func.type().args()
        for i in range(0, len(ids)):
            addr = self.llvmAddrLocalVar(func, callee_frame, ids[i].name())
            self.llvmInit(llvm_func.args[i], addr)
            
            # Make call.
        callee_func = self.llvmGetFunction(func)
        result = self.builder().call(callee_func, [callee_frame])
        result.calling_convention = llvm.core.CC_FASTCALL

            # Set exception parameter.
        addr = self.llvmAddrException(stub_frame)
        excpt = self.builder().load(addr)
        self.builder().store(excpt, llvm_func.args[-1])

            # If we have an exception, save the stub frame in case we are going
            # resume later. 
        raised = self.builder().icmp(llvm.core.IPRED_NE, excpt, self.llvmConstNoException())
    	block_excpt = llvm_func.append_basic_block(self.nameNewLabel("excpt"))
        block_noexcpt = llvm_func.append_basic_block(self.nameNewLabel("noexcpt"))
        self.builder().cbranch(raised, block_excpt, block_noexcpt)
    
        self.pushBuilder(block_excpt)
        exception_save_frame = codegen.llvmCurrentModule().get_function_named("__hlt_exception_save_frame")
        stub_frame_casted = self.builder().bitcast(stub_frame, llvm.core.Type.pointer(self.llvmTypeBasicFrame()))
        self.builder().call(exception_save_frame, [excpt, stub_frame_casted])
        self.builder().branch(block_noexcpt)
        self.popBuilder()
    
        self.pushBuilder(block_noexcpt)
        
            # Load return value.
        if not is_void:
            zero = self.llvmGEPIdx(0)
            index = self.llvmGEPIdx(len(self._bf_fields))
            addr = self.builder().gep(stub_frame, [zero, index])
            val = self.builder().load(addr)
            self.builder().ret(val)
            
        else:
            self.builder().ret_void()

        self.popBuilder() # block_noexcp
        self.popBuilder() # function body.

        primary = llvm_func
        
        #### Create the resume function. 
        if not is_void:
            result_type = self.llvmTypeConvert(func.type().resultType())
            addl_arg = [result_type]
        else:
            result_type = llvm.core.Type.void()
            addl_arg = []
        
        rt = result_type
        args = [self.llvmTypeExceptionPtr(), llvm.core.Type.pointer(self.llvmTypeExceptionPtr())]
        ft = llvm.core.Type.function(rt, args)
        llvm_func = self._llvm.module.add_function(ft, name + "_resume")
        llvm_func.calling_convention = llvm.core.CC_C
        self._llvm.functions[name] = llvm_func
        block = llvm_func.append_basic_block("")
        self.pushBuilder(block)

            # Get the contination values.
        excpt = llvm_func.args[0]
        excpt = self.builder().bitcast(excpt, self.llvmTypeGenericPointer())
        exception_get_continuation = codegen.llvmCurrentModule().get_function_named("__hlt_exception_get_continuation")
        cont = self.builder().call(exception_get_continuation, [excpt])
        
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1) 
        succ_addr = self.builder().gep(cont, [zero, zero])
        succ_addr = self.builder().bitcast(succ_addr, llvm.core.Type.pointer(self.llvmTypeBasicFunctionPtr()))

        frame_addr = self.builder().gep(cont, [zero, one])
        frame_addr = self.builder().bitcast(frame_addr, llvm.core.Type.pointer(llvm.core.Type.pointer(self.llvmTypeBasicFrame())))

        succ = self.builder().load(succ_addr)
        frame = self.builder().load(frame_addr)

            # Restore the stub_frame
        exception_restore_frame = codegen.llvmCurrentModule().get_function_named("__hlt_exception_restore_frame")
        stub_frame = self.builder().call(exception_restore_frame, [excpt])
        
            # Clear exception information.
        addr = self.llvmAddrException(stub_frame)
        zero = self.llvmConstNoException()
        self.llvmInit(zero, addr)
        
            # Make call.
        result = self.builder().call(succ, [frame])
        result.calling_convention = llvm.core.CC_FASTCALL

            # Set exception parameter.
        addr = self.llvmAddrException(stub_frame)
        excpt = self.builder().load(addr)
        self.builder().store(excpt, llvm_func.args[-1])

            # If we have an exception, save the stub frame in case we are going
            # resume later. 
        raised = self.builder().icmp(llvm.core.IPRED_NE, excpt, self.llvmConstNoException())
    	block_excpt = llvm_func.append_basic_block(self.nameNewLabel("excpt"))
        block_noexcpt = llvm_func.append_basic_block(self.nameNewLabel("noexcpt"))
        self.builder().cbranch(raised, block_excpt, block_noexcpt)
    
        self.pushBuilder(block_excpt)
        exception_save_frame = codegen.llvmCurrentModule().get_function_named("__hlt_exception_save_frame")
        stub_frame_casted = self.builder().bitcast(stub_frame, llvm.core.Type.pointer(self.llvmTypeBasicFrame()))
        self.builder().call(exception_save_frame, [excpt, stub_frame_casted])
        self.builder().branch(block_noexcpt)
        self.popBuilder()
    
        self.pushBuilder(block_noexcpt)
        
            # Load return value.
        if not is_void:
            zero = self.llvmGEPIdx(0)
            index = self.llvmGEPIdx(len(self._bf_fields))
            stub_frame_casted = self.builder().bitcast(stub_frame, llvm.core.Type.pointer(frame_type))
            addr = self.builder().gep(stub_frame_casted, [zero, index])
            val = self.builder().load(addr)
            self.builder().ret(val)
            
        else:
            self.builder().ret_void()

        self.popBuilder() # block_noexcept builder.
        self.popBuilder() # Function builder. 

        resume = llvm_func
        
        return (primary, resume)
        
    def llvmRaiseException(self, exception):
        """Generates the raising of an exception. The method
        uses the current :meth:`builder`.
        
	    exception: llvm.core.Value - A value with the exception.
        """
    
        # ptr = *__frame.bf.cont_exception.succesor
        fpt = llvm.core.Type.pointer(self.llvmTypeBasicFunctionPtr())
        addr = self.llvmAddrExceptContSuccessor(self._llvm.frameptr, fpt)
        ptr = self.builder().load(addr, "succ_ptr")
    
        # frame = *__frame.bf.cont_exception
        bfptr = llvm.core.Type.pointer(self.llvmTypeBasicFrame())
        addr = self.llvmAddrExceptContFrame(self._llvm.frameptr, llvm.core.Type.pointer(bfptr))
        frame = self.builder().load(addr, "frame_ptr")
        
        # frame.exception = <exception>
        addr = self.llvmAddrException(frame)
        self.llvmInit(exception, addr)
        
        self.llvmGenerateTailCallToFunctionPtr(ptr, [frame])
        
    def _llvmNewException(self, etype, arg, location):
        if not arg:
            arg = llvm.core.Constant.null(self.llvmTypeGenericPointer())
            
        exception_new = codegen.llvmCurrentModule().get_function_named("__hlt_exception_new")
        arg = self.builder().bitcast(arg, self.llvmTypeGenericPointer())
        location = self.llvmAddGlobalStringConst(str(location), "excpt")
        
        return self.builder().call(exception_new, [etype, arg, location])
        
    def llvmNewException(self, type, arg, location):
        """Creates a new exception object. The method uses the current
        :meth:`builder`.
        
	    type: type.Exception - The type of the exception to create.
        
        arg: llvm.core.Value - The exception argument if exception takes one,
        or None otherwise. 
        
        location: Location - A location to associate with the exception.
        
        Returns: llvm.core.Value - The new exception object.
        """
        assert (type.argType() and arg) or (not type.argType() and not arg)
        etype = codegen.llvmAddExceptionType(type)
        return self._llvmNewException(etype, arg, location)
        
    def llvmRaiseExceptionByName(self, exception, location, arg=None):
        """Generates the raising of an exception given by name. The method
        uses the current :meth:`builder`.
        
	    exception: string - The name of the internal global variable
        representing the exception *type* to raise. These names are defined in 
        |hilti.h|.
        
        location: Location - A location to associate with the exception.

        arg: llvm.core.Value - The exception argument if exception takes one,
        or None otherwise. 
        """
        etype = self.llvmGetGlobalVar(exception, self.llvmTypeExceptionType(), ptr=True)
        excpt = self._llvmNewException(etype, arg, location)
        self.llvmRaiseException(excpt)
        
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

    def _llvmGenerateTailCall(self, llvm_func, args, llvm_result=None):
        result = self.builder().call(llvm_func, args)
        result.calling_convention = llvm.core.CC_FASTCALL
        result.tail_call = True
        
        if not llvm_result:
            self.builder().ret_void()
        else:
            val = self.builder().load(llvm_result)
            self.builder().ret(val)

        # Any subsequent code generated inside the same block will
        # be  unreachable. We create a dummy builder to absorb it; the code will
        # not appear anywhere in the output.
        self.popBuilder();
        self.pushBuilder(None)
        
    def llvmGenerateTailCallToBlock(self, block, args):
        """Generates a tail call to a block's function. We generate a separate
        LLVM function for each ~~Block, which can then be called with via this
        method. The method uses the current :meth:`builder`.
        
        block: ~~Block - The block to call the function for. 
        args: list of llvm.core.Value - The arguments to pass to the function.
        """
        llvm_func = self.llvmGetFunctionForBlock(block)
        self._llvmGenerateTailCall(llvm_func, args)
            
    def llvmGenerateTailCallToFunction(self, func, args, llvm_result=None):
        """Generates a tail call to a function. The function must have
        ~~Linkage ~HILTI. The method uses the current :meth:`builder`. 
        
        function: ~~Function - The function, which must have ~~HILTI ~~Linkage.
        args: list of llvm.core.Value - The arguments to pass to the function.
        """
        assert func.callingConvention() == function.CallingConvention.HILTI
        llvm_func = self.llvmGetFunction(func)
        assert llvm_func
        self._llvmGenerateTailCall(llvm_func, args, llvm_result=llvm_result)

    def llvmGenerateTailCallToFunctionPtr(self, ptr, args):
        """Generates a tail call to a function pointer. The function being
        pointed to must have ~~Linkage ~HILTI. The method uses the current
        :meth:`builder`. 
        
        function: ~~llvm.core.Pointer - The pointer to a function. 
        args: list of llvm.core.Value - The arguments to pass to the function.
        """
        self._llvmGenerateTailCall(ptr, args)

    def llvmConstDefaultValue(self, type):
        """Returns a type's default value for uninitialized values. The type
        must be a constant ~~ValueType.
        
        type: ValueType - The type to determine the default value for.
        
        Returns: llvm.core.Constant - The default value.
        """
        default = self._callCallback(CodeGen._CB_DEFAULT_VAL, type, [type])
        assert default
        llvm.core.check_is_constant(default)
        return default
        
    def llvmInit(self, value, addr):
        """Initializes a formerly unused address with a value. This
        essentially acts like a ``builder().store(value, addr)``. However, it
        must be used instead of that to indicate that a formerly
        non-initialized variable is being written to the first time. 
        
        Note: Currently, this is in fact just a ``store()`` operation. In the
        future however we might add more logic here, e.g., if using reference
        counting.
        
        Todo: This is not called for function parameters/returns yet.
        """
        self.builder().store(value, addr)

    def llvmInitWithDefault(self, type, addr):
        """Initializes a formerly unused address with a type's default value.
        The type must be a ValueType. 
        
        type: ValueType - The type determining the default value. 
        addr: llvm.core.Value - The address to initialize. 
        """
        self.llvmInit(self.llvmConstDefaultValue(type), addr)
        
    def llvmAssign(self, value, addr):
        """Assignes a value to an address that has already been initialized
        earlier. This essentially acts like a ``builder().store(value,
        addr)``. However, it must be used instead of that to indicate that a
        formerly initialized variable is being changed. 
        
        Note: Currently, this is in fact just a ``store()`` operation. In the
        future however we might add more logic here, e.g., if using reference
        counting.
        
        Todo: This is not called for function parameters/returns yet.
        """
        self.builder().store(value, addr)
            
    def llvmMalloc(self, type):
        """Allocates memory for the given type. This essentially acts like a
        ``builder().malloc(type)``. However, it must be used instead of
        that in case further tasks are associated with such a malloc.

        Note: Currently, this is in fact just a ``malloc()`` operation. In the
        future however we might add more logic here, e.g., if using garbage
        collection.
        """
        return self.builder().malloc(type)
    
    def llvmStoreInTarget(self, target, val):
        """Stores a value in a target operand. The method uses the current
        :meth:`builder`. 
        
        target: ~~IDOperand - The target operand. 
        val: llvm.core.Value - The value to store.
        
        Todo: Currently, stores are only supported for local variables, not
        globals.
        """
        if isinstance(target, instruction.IDOperand):
            i = target.id()
            if i.role() == id.Role.LOCAL or i.role() == id.Role.PARAM:
                # A local variable.
                addr = self.llvmAddrLocalVar(self._function, self._llvm.frameptr, i.name())
                self.llvmAssign(val, addr)
                return 
            
            if i.role() == id.Role.GLOBAL:
                # A local variable.
                addr = self.llvmAddrGlobalVar(i)
                self.llvmAssign(val, addr)
                return 

        if isinstance(target, instruction.LLVMOperand):
            self.llvmAssign(val, target.value())
            return
            
        util.internal_error("llvmStoreInTarget: unknown target class: %s" % target)
        
    def llvmStoreTupleInTarget(self, target, vals):
        """Stores a tuple value in a target operand. The method uses the current
        :meth:`builder`. 
        
        target: ~~IDOperand - The target operand. 
        vals: list of llvm.core.Value - The elements of the tuple to store.
        
        Todo: Currently, stores are only supported for local variables, not
        globals.
        """

        if isinstance(target, instruction.IDOperand):
            i = target.id()
            if i.role() == id.Role.LOCAL or i.role() == id.Role.PARAM:
                # A local variable.
                addr = self.llvmAddrLocalVar(self._function, self._llvm.frameptr, i.name())
                
                for i in range(len(vals)):
                    field = self.builder().gep(addr, [self.llvmGEPIdx(0), self.llvmGEPIdx(i)])
                    self.llvmAssign(vals[i], field)
                    
                return 
        
        util.internal_error("llvmStoreTupleInTarget: unknown target class: %s" % target)

    def llvmAddGlobalVar(self, value, tag):
        """Adds a global variable to the current module. The variable is
        initialized with the given value, and will have only internal linkage.
        
        value: llvm.core.Value - The value to initialize the variable with.
        tag: string - A tag which will be added the variable's name for easier
        identification in the generated bitcode.
        
        Return: llvm.core.Value - A value representing the new variable.
        """
        name = self.nameNewGlobal(tag)
        glob = codegen.llvmCurrentModule().add_global_variable(value.type, name)
        glob.initializer = value
        glob.linkage = llvm.core.LINKAGE_INTERNAL
        return glob
        
        
    def llvmAddGlobalConst(self, value, tag):
        """Adds a global constant to the current module. The constant is
        initialized with the given value, and will have only internal linkage.
        
        value: llvm.core.Value - The value to initialize the constant with.
        tag: string - A tag which will be added the constant's name for easier
        identification in the generated bitcode.
        
        Return: llvm.core.Value - A value representing the new constant.
        """
        glob = self.llvmAddGlobalVar(value, tag)
        glob.global_constant = True
        return glob
    
    def llvmAddGlobalStringConst(self, s, tag): 
        """Adds a global string constant to the current module. The constant
        is initialized with the given string and null-terminated; it will have
        internal linkage and C type ``const char *``.
        
        s: string - The string to initialize the constant with.
        tag: string - A tag which will be added the constant's name for easier
        identification in the generated bitcode.
        
        Return: llvm.core.Value - A value representing the new constant.
        """
        s += '\0'
        bytes = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in s]
        array = llvm.core.Constant.array(llvm.core.Type.int(8), bytes)
        return self.llvmAddGlobalConst(array, tag).bitcast(self.llvmTypeGenericPointer())

    def _llvmGenerateCtors(self):
        # Generate the global ctor table. Must be called only once per module
        # (and obviously after all ctors have been registered.)
        #
        # Note: The way this works in LLVM doesn't appear to be documented.
        # But here's how I understand it: One defines a global array called
        # ``llvm.global_ctors`` of type ``[ <n> x { i32, void ()* }]``. The
        # first element of each tuple is a priority defining the order in which
        # multiple ctors are run; however LLVM backends seem to ignore this at
        # the moment and it is suggested to simply set it to 65536. The second
        # element of each tuple is a pointer to a function ``void f()`` which
        # will be called on startup. This array must have linkage ``appending``
        # so that multiple of them (potentially in different compilation units)
        # will be combined into one.

        array = [llvm.core.Constant.struct([self.llvmConstInt(65536, 32), func]) for func in self._llvm.global_ctors]
        
        if not array:
            return

        funct = llvm.core.Type.function(llvm.core.Type.void(), [])
        structt = llvm.core.Type.struct([llvm.core.Type.int(32), llvm.core.Type.pointer(funct)])
        array = llvm.core.Constant.array(structt, array)
        
        glob = codegen.llvmCurrentModule().add_global_variable(array.type, "llvm.global_ctors")
        glob.global_constant = True
        glob.initializer = array
        glob.linkage = llvm.core.LINKAGE_APPENDING
        
    def llvmAddGlobalCtor(self, callback):
        """Registers code to be executed at program startup. This is primarily
        intended for registering constructors for global variables, which will
        then be run before any other code is executed. 
        
        The code itself needs to be generated by the *callback*, which should
        use the current ~~builder. *callback* will not receive any arguments.
        
        callback: Function - Function receiving no parameters.
        """

        funct = llvm.core.Type.function(llvm.core.Type.void(), [])
        function = llvm.core.Function.new(self.llvmCurrentModule(), funct, self.nameNewFunction("ctor"))
        block = function.append_basic_block("")
        
        # Need to create a new list to save the old builders here.
        save_builders = [b for b in self._llvm.builders]
        try:
            save_current_func = self._llvm.func
        except AttributeError:
            save_current_func = None
            
        self._llvm.func = function
        
        self.pushBuilder(block)
        callback()
        self.builder().ret_void()
        
        self.popBuilder()
        self._llvm.func = save_current_func
        self._llvm.builders = save_builders
        
        self._llvm.global_ctors += [function]
        
    # Table of callbacks. 
    _CB_TYPE_INFO = 1
    _CB_CTOR_EXPR = 2
    _CB_LLVM_TYPE = 3
    _CB_DEFAULT_VAL = 4
    _CB_OPERATOR = 5
    _CB_UNPACK = 6
    
    _Callbacks = { 
    	_CB_TYPE_INFO: [], 
        _CB_CTOR_EXPR: [], 
        _CB_LLVM_TYPE: [], 
        _CB_DEFAULT_VAL: [],
        _CB_OPERATOR: [],
        _CB_UNPACK: []
        }
        
    def _callCallback(self, kind, type, args, must_find=True):
        
        assert kind != CodeGen._CB_OPERATOR # Use llvmExecuteOperator() instead.
        
        for (t, func) in CodeGen._Callbacks[kind]:
            if isinstance(type, t):
                return func(*args)
            
        if must_find:
            util.internal_error("_callCallback: unsupported object %s for callback type %d" % (repr(type), kind))
            
        return None
    
    def llvmExecuteOperator(self, op):
        """Generates code for a type-specific operator. 
        
        op: ~~Operator - The operator to generate code for.
        """

        ovops = instruction.findOverloadedOperator(op)
        assert len(ovops) == 1
        ovop = ovops[0]

        for (o, func) in CodeGen._Callbacks[CodeGen._CB_OPERATOR]:
            if repr(o) == repr(ovop):
                func(self, op)
                return True
        
        util.internal_error("llvmExecuteOperator: no implementation found for %s" % op)

    def llvmUnpack(self, t, begin, end, fmt, arg=None):
        """
        Unpacks an instance of a type from binary data. 
        
        t: ~~HiltiType - The type of the object to be unpacked from the input. 
        
        begin: llvm.core.value - The byte iterator marking the first input
        byte.
        
        end: llvm.core.value - The byte iterator marking the position one
        beyond the last consumable input byte. *end* may be None to indicate
        unpacking until the end of the bytes object is encounted.
        
        fmt: ~~Operand or other - Specifies the binary format of the input bytes. It
        can be either an ~~Operand of ~~EnumType ``Hilti::Packed``, a string
        specifying the name of a ``Hilti::Packed`` label, or an integer with
        the internal value of the desired label. Finally, it can be an
        ``llvm.core.Constant`` if that constant has been instantiated by 
        ~~llvmOp (and only then!).
        
        arg: optional llvm.core.Value - Additional format-specific parameter,
        required by some formats. 
        
        Returns: tuple (val, iter) - A Python tuple in which *val* is the
        unpacked value of type ``llvm.core.value`` and ``iter`` is an
        ``llvm.core.value`` corresponding to a byte iterator pointing one
        beyond the last consumed input byte (which can't be further than
        *end*). 
        """

        assert not inspect.isclass(t)
        
        packed = self._module.lookupID("Hilti::Packed")
        assert packed and isinstance(packed.type(), type.TypeDeclType)
        
        packed_t = packed.type().declType()
        assert isinstance(packed_t, type.Enum)
        
        val = -1
        
        if isinstance(fmt, llvm.core.Constant):
            try:
                fmt = fmt._value
            except AttributeError:
                util.internal_error("llvmUnpack: LLVM constant not instantiated by llvmOp")
                
        elif isinstance(fmt, llvm.core.Value):
            util.internal_error("llvmUnpack: LLVM value must be constant")
        
        if isinstance(fmt, str):
            id = self._module.lookupID(fmt)
            if not id:
                util.internal_error("llvmUnpack: %s is not a known ID" % fmt)
            
            if not id.type() is packed_t:
                util.internal_error("llvmUnpack: %s is not a label of Hilti::Packed" % fmt)
                
            val = self._module.lookupIDVal(fmt)
            assert val

        if isinstance(fmt, int):
            val = fmt
            
        if val >= 0:
            fmt = instruction.ConstOperand(constant.Constant(val, packed_t))

        if not end:
            import bytes
            end = bytes.llvmEnd()
            
        assert isinstance(fmt, instruction.Operand) and fmt.type() is packed_t
        
        if isinstance(t, type.Reference):
            # When unpacking a reference, we actually unpack an object of the
            # typed begin referenced.
            t = t.refType()
        
        return self._callCallback(CodeGen._CB_UNPACK, t, [t, begin, end, fmt, arg])
        
    def llvmOp(self, op, refine_to = None):
        """Converts an instruction operand into an LLVM value. The method
        might use the current :meth:`builder`. 
        
        op: ~~Operand - The operand to be converted.
        
        refine_to: ~~Type - An optional refined type to convert the operand to
        first. If given, the code converter can use it to produce a more
        specific type than the operand yields by itself. The typical use case
        is that *op* comes with a wilcard type (i.e., ``ref<*>``) that it
        supposed to be used with a specific instantiation (i.e., ``ref<T>``)
        and thus needs to be adapted first. *refine_to* must match with *op's*
        type (i.e., ``refine_type == op.type()`` must evaluate to true).
        
        Returns: llvm.core.Value - The LLVM value of the operand that can then
        be used, e.g., in subsequent LLVM load and store instructions.
        
        Todo: We currently support only operands type ~~IDOperand,
        ~~ConstOperand, and ~~TupleOperand; and for ~~IDOperand only those
        referencing local variables.
        """

        assert (not refine_to) or (refine_to == op.type())
        
        t = op.type()
        
        if isinstance(op, instruction.ConstOperand):
            val = self._callCallback(CodeGen._CB_CTOR_EXPR, t, [op, refine_to])
            # For references, constants are in fact a pointer to the const so we
            # need an additional dereference. See, e.g., regexp.llvmCtorExpr.
            # op.value() is None for the Null reference, which must not
            # reference.
            if isinstance(t, type.Reference) and op.value() != None:
                val = self.builder().load(val, "const-deref")
                
            val._value = op.value()  # Used in llvmUnpack()
            return val
        
        if isinstance(op, instruction.TupleOperand):
            return self._callCallback(CodeGen._CB_CTOR_EXPR, t, [op, refine_to])

        if isinstance(op, instruction.IDOperand):
            i = op.id()
            if i.role() == id.Role.LOCAL or i.role() == id.Role.PARAM:
                # A local variable.
                addr = self.llvmAddrLocalVar(self._function, self._llvm.frameptr, i.name())
                return self.builder().load(addr, "op")
            
            if i.role() == id.Role.GLOBAL:
                # A (thread-local) global variable.
                addr = self.llvmAddrGlobalVar(i)
                return self.builder().load(addr, "op")

            if i.role() == id.Role.CONST:
                # A global constant. 
                init = self.currentModule().lookupIDVal(i)
                # If init is an LLVM value, it represents the constant that
                # we already have created previously.
                if isinstance(init, llvm.core.Value):
                    
                    if isinstance(t, type.Reference):
                        init = self.builder().load(init, "const-deref")
                        
                    return self.builder().load(init, "const")

                # Create the constant.
                assert init and isinstance(init, instruction.ConstOperand)
                val = self._callCallback(CodeGen._CB_CTOR_EXPR, t, [init, t])
                const = self.llvmAddGlobalConst(val, i.name())

                self.currentModule().addID(i, const) # Save the value for later. 

                # Again, for references, constants are in fact a pointer to
                # the const. Note that we can't use llvmOp() recursively here
                # because the LLVM constant must have a const initializer. Do
                # this *after* saving the value as otherwise we'd save a local
                # tmp.
                if isinstance(t, type.Reference):
                    const = self.builder().load(const, "const-deref")
                
                return self.builder().load(const, "const")
            
            util.internal_error("llvmOp: unsupported ID operand type %s" % t)
            
        if isinstance(op, instruction.TypeOperand):
            return self.llvmTypeInfoPtr(op.value())

        if isinstance(op, instruction.LLVMOperand):
            return op.value()
        
        util.internal_error("llvmOp: unsupported operand type %s" % repr(op))

    def llvmTypeConvert(self, type, refine_to = None):
        """Converts a ValueType into the LLVM type used for corresponding
        variable declarations.
        
        type: ~~ValueType - The type to converts.
        
        refine_to: ~~Type - An optional refined type to convert the operand to
        first. See ~~llvmOp for more information.
        
        Returns: llvm.core.Type - The corresponding LLVM type for variable declarations.
        """ 
        assert (not refine_to) or (refine_to == op.type())
        return self._callCallback(CodeGen._CB_LLVM_TYPE, type, [type, refine_to])

    def llvmEnumLabel(self, label):
        """Looks up the internal value of an enum label.
        
        label: string - The name of the label in qualified form, i.e., ``a::b``.
        
        Returns: llvm.core.Constant - The LLVM value corresponding to *label*.
    
        Note: The functions will abort execution if *label* is not known.
        """
        v = self._module.lookupID(label)
        
        if not v or not isinstance(v.type(), type.Enum):
            util.internal_error("%s is not a valid enum label" % label)
            
        return self.llvmConstInt(self._module.lookupIDVal(label), 8)
    
    def llvmExtractValue(self, struct, idx):
        """Extract a specific index from an LLVM struct. This function
        works-around the fact that llvm-py at the moment does not support
        LLVM's ``extractvalue``. Once it does, we should change the
        implementation accordingly.
        
        struct: llvm.core.Value - An LLVM value of type
        ``llvm.core.Type.struct`` (not a pointer; use GEP for pointers).
        
        idx: integer or llvm.core.Value - The zero-based index to extract,
        either as a constant integer, or as LLVM value of integer type.
        
        Returns: llvm.core.Value - The value of the indexed field.
        
        Todo: The current implementation is awful. Really need the
        ``extractvalue`` instruction.
        """
        
        addr = codegen.llvmAlloca(struct.type)
        codegen.builder().store(struct, addr)
        
        if isinstance(idx, int):
            idx = codegen.llvmGEPIdx(idx)
        
        ptr = self.builder().gep(addr, [self.llvmGEPIdx(0), idx])
        return self.builder().load(ptr)
    
    def llvmInsertValue(self, obj, idx, val):
        """Inserts a value at a specific index in an LLVM struct or array.
        This function works-around the fact that llvm-py at the moment does
        not support LLVM's ``insertvalue``. Once it does, we should change
        the implementation accordingly.
        
        obj llvm.core.Value - An LLVM value of type ``llvm.core.Type.struct``
        or ``llvm.core.Type.array`` (not a pointer; use GEP for pointers).
        
        idx: integer or llvm.core.Value - The zero-based index to extract,
        either as a constant integer, or as LLVM value of integer type.
        
        val: llvm.core.Value - The new value for the field.
        
        Returns: llvm.core.Value - A value of the same type as *obj* with the
        specified field replaced. 
        
        Todo: The current implementation is awful. Really need the
        ``insertvalue`` instruction.
        """
        
        addr = codegen.llvmAlloca(obj.type)
        codegen.builder().store(obj, addr)
        
        if isinstance(idx, int):
            idx = codegen.llvmGEPIdx(idx)
        
        ptr = self.builder().gep(addr, [self.llvmGEPIdx(0), idx])
        self.builder().store(val, ptr)
        return self.builder().load(addr)

    def llvmSwitch(self, op, cases):
        """Helper to build an LLVM switch statement for integer and enum
        operands. For each case, the caller provides a function that will be
        called to build the case's body. The helper will build the necessary
        code around the switch, generate an exception if the switch operand
        comes with an unknown case, and push a builder on the stack for code
        following the switch statement. If the switch operand is a constant,
        only the relevant case body will be emitted. 
        
        *op*: ~~Operand - The operand to switch on; *op* must be either of
        type ~~Integer or of type ~~Enum.
        
        *cases: list of tuples (constant, function) - Each constant specifies
        a case's value for *op* and must be either a Python integer or a
        string with the fully qualified name of an enum label; the
        corresponding *function* will be called be the code generator to emit
        the LLVM code for that case. The function will be called with one
        parameter, the *constant*; and it should use the current ~~builder to
        emit IR. Optionally, *constant* can also be a list of such constants,
        and the corresponding block will then be executed for all of the cases
        they specify; in this case, *function* will only be called for the
        first entry of the list. 
        """
        assert isinstance(op.type(), type.Integer) or isinstance(op.type(), type.Enum)
        
        if isinstance(op, instruction.ConstOperand):
            # Version for a const operand. 
            for (vals, function) in cases:
                if not isinstance(vals, list):
                    vals = [vals]
                    
                for val in vals:
                    if isinstance(val, str):
                        val = self._module.lookupIDVal(val)
                        if not val:
                            util.internal_error("llvmSwitch: %s is not a know enum value" % val)
                            
                    if val == op.value():
                        function(val)
                        return
                    
            util.internal_error("codegen.llvmSwitch: constant %s does not specify a known case" % op.value())
        
        else:
            # Version for a non-const operand. 
            default = self.llvmNewBlock("switch-default")
            self.pushBuilder(default) 
            self.llvmRaiseExceptionByName("hlt_exception_value_error", op.location())
            self.popBuilder() 
        
            builder = self.builder()
            cont = self.llvmNewBlock("after-switch")
        
            llvmop = self.llvmOp(op)
            switch = builder.switch(llvmop, default)
        
            for (vals, function) in cases:
                if not isinstance(vals, list):
                    vals = [vals]
                    
                block = self.llvmNewBlock("switch-%s" % str(vals[0]))
                self.pushBuilder(block)
                function(vals[0])
                self.builder().branch(cont)
                self.popBuilder()
                
                for val in vals:
                    
                    if isinstance(val, str):
                        val = self._module.lookupIDVal(val)
                        if not val:
                            util.internal_error("llvmSwitch: %s is not a know enum value" % val)
                    
                    switch.add_case(llvm.core.Constant.int(llvmop.type, val), block)
        
            self.pushBuilder(cont) # Leave on stack.

    def llvmCallIntrinsic(self, intr, types, args):
        """Calls an LLVM intrinsic. 
        
        intr: integer - The intrinsic's ``INTR_*`` constant as defined in
        ``llvm.core``.
        
        types - list of llvm.core.Type - The types by which the intrinsic is
        overloaded. 
        
        args - list of llvm.core.Value - The arguments to call the intrinsic
        with.
        
        Returns: llvm.core.Value - Whatever the intrinsic returns.
        """
           
        i = llvm.core.Function.intrinsic(self._llvm.module, intr, types)
        return self.builder().call(i, args)

    ### Decorators.
    
    def typeInfo(self, t):
        """Decorator to define a function creating a type's meta
        information. The decorated function will receive a single parameter of
        type ~~HiltiType and must return a suitable ~~TypeInfo object or None
        if it does not want type information to be created for that type.
        
        t: ~~HiltiType - The type for which the ~~TypeInfo is being
        requested.
        """
        def register(func):
            assert issubclass(t, type.HiltiType)
            CodeGen._Callbacks[CodeGen._CB_TYPE_INFO] += [(t, func)]
    
        return register

    def llvmCtorExpr(self, type):
        """Decorator to define a conversion from a value as created by a
        type's constructor expression to the corresponding LLVM value. 
        Constructor expressions are defined in the ~~parser.  The decorated
        function receives two parameters, *type* and *refine_to*.  The
        former's type depends on what the parser instantiates; the latter
        specifies a more specific type to covert *type* to first and may be
        None (see ~~llvmOp for more details about *refine_to*).
        
        Many types will only have constructor expressions for their constants
        (e.g., numbers for the integer data type, which will be passed in as
        Python ints; character sequences enclosed in quotation marks for
        strings, which will be passed in as Python strings). An example of
        non-constant constructor expressions are tuples: ``(x,y,z)`` will be
        passed in a list of ~~Operand objectives.
        
        The conversion function must return an `llvm.core.Value`` and can use
        the current :meth:`builder` if it needs to perform any transformations
        on the operand. 
        
        type: ~~ValueType - The type for which the conversion is being defined.
        """
        def register(func):
            CodeGen._Callbacks[CodeGen._CB_CTOR_EXPR] += [(type, func)]
    
        return register
    
    def llvmType(self, type):
        """Decorator to define a conversion from a ValueType to the
        corresponding type used in LLVM code.  The decorated function receives
        two parameters, *type* and *refine_to*.  The former is an instance of
        ~~Type to convert; the latter specifies a more specific type to covert
        *type* to first and may be None (see ~~llvmOp for more details about
        *refine_to*). The decorated function must return an
        ``llvm.core.Type``..
        
        type: ~~ValueType - The type for which the conversion is being defined.
        
        Note: The type defined here will also be the only passed to C functions.
        """
        def register(func):
            CodeGen._Callbacks[CodeGen._CB_LLVM_TYPE] += [(type, func)]
    
        return register
            
    def llvmDefaultValue(self, type):
        """Decorator to generate a default value for uninitialized values. 
        All ~~ValueTypes must define such a default value. The decorated
        function receives one parameter, the ~~Type of the value to
        initialize; and it must return an ``llvm.core.Value`` representing the
        type's default value, which must be a constant. 
                
        type: ~~ValueType - The type for which the conversion is being defined.
        """
        def register(func):
            CodeGen._Callbacks[CodeGen._CB_DEFAULT_VAL] += [(type, func)]
    
        return register    
    
    def operator(self, op):
        """Decorator to define the type-specific implementation of an
        operator. The decorated function receives two paramemter: the first is
        the ~~CodeGen, and the second an ~~Operator; for the operator,
        ~~operatorType() is guarenteed to match *type*. The decorated function
        must generate LLVM code corresponding to *type*'s implementation of
        the operator. 
        
        op: ~~Operator class - The operator to define an implementation for.
        """
        def register(func):
            #assert isinstance(op, instruction.OverloadedOperator)
            CodeGen._Callbacks[CodeGen._CB_OPERATOR] += [(op, func)]
    
        return register    
    
    def unpack(self, t):
        """Decorator to define the implementation of the ~~Unpack
        operator for a specific type. The decorated function must
        emit LLVM code instantiating an object of type *t* from
        binary data stored in a ~~Bytes object. The function will
        receive four parameters: the type *t* it is supposed to
        unpack; two ``llvm.core.value`` objects containing byte
        iterators *begin* and *end, respectively, defining the range
        of bytes to use for unpacking; and an ~~Operand *op* of
        ~~EnumType ``Hilti::Packed`` specifying the format of the
        input bytes. The function does not need to consume all bytes
        starting at *start* but it must not consume any beyond
        *end*. The function must return a Python tuple ``(val,
        iter)`` in which *val* is the unpacked value of type
        ``llvm.core.value`` and ``iter`` is an ``llvm.core.value``
        corresponding to a byte iterator pointing one beyond the
        last consumed input byte (which can't be further than
        *end*). If the function does not support the provided *fmt*,
        it should raise an ~~WrongArguments exception.
        
        The decorated function should have a doc string documenting
        the specifics of the unpacking, in partciular including
        which ``Hilti::Packed`` constants are supported.
        
        t: ~~Type - The type for which unpacking is implemented by the
        decorator function. 
        
        Note: Different from most other operators, unpacking comes with its
        own decorator because it is also used internally, not only via the
        corresponding HILTI instruction. This decorator allows to use a single
        implementation for all use-cases. 
        
        Also note that all unpacking functions must be able to deal with bytes
        located at arbitrary and not necessarily aligned positions.
        
        Todo: The doc-strings of the decorated functions should be
        inserted into the documentation automatically.
        """
        def _unpack(func):
            CodeGen._Callbacks[CodeGen._CB_UNPACK] += [(t, func)]
    
        return _unpack
    
codegen = CodeGen()

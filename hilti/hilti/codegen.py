#! /usr/bin/env
#
# LLVM code geneator.

builtin_id = id

import sys
import copy

import llvm
import llvm.core 

import inspect

import block
import function
import id
import instruction
import objcache
import operand
import system
import type
import util

class _DummyBuilder(object):
    """Class which just absorbs any method calls."""
    def __getattribute__(*args):
        class dummy:
            def __call__(*args):
                pass
            
        return lambda *args: dummy()

    
class TypeInfo(object):
    """Represent's type's meta information. The meta information is
    used to describe a type's properties, such as hooks and
    pre-defined operators. A subset of information from the TypeInfo
    objects will be made available to libhilti during run-time; see
    ``struct hlt_type_information`` in
    :download:`/libhilti/hilti_intern.h`. There must be one TypeInfo
    object for each ~~HiltiType instance, and the usual place for
    creating it is in a type's ~~typeInfo.
    
    A TypeInfo object has the following fields, which can be get/set directly:

    *type* (~~HiltiType):
       The type this object describes.
    
    *name* (string)
       A readable representation of the type's name.
       
    *c_prototype* (string) 
       A string specifying how the type should be represented in C function
       prototypes.
       
    *to_string* (string)
       The name of an internal libhilti function that converts a value of the
       type into a readable string representation. See :download:`/libhilti/hilti_intern.h`
       for the function's complete C signature. 
       
    *to_int64* (string) 
       The name of an internal libhilti function that converts a value of the
       type into an int64. See :download:`/libhilti/hilti_intern.h` for the
       function's complete C signature. 
       
    *to_double* (string)
       The name of an internal libhilti function that converts a value of the
       type into a double. See :download:`/libhilti/hilti_intern.h`
       for the function's complete C signature. 
       
    *hash* (string) 
       The name of an internal libhilti function that calculates a
       hash value for an instance of the type. If not given, a
       default is used that compares value's byte-wise. See
       :download:`/libhilti/hilti_intern.h` for the function's
       complete C signature. 

    *equal* (string)
       The name of an internal libhilti function that compares to
       instances of the type. If not given, a default is used that
       compares value's byte-wise. See
       :download:`/libhilti/hilti_intern.h` for the function's
       complete C signature. 
       
    *aux* (llvm.core.Value)
       A global LLVM variable with auxiliary type-specific information, which
       will be accessible from the C level.
              
    t: ~~HiltiType - The type used to initialize the object. The fields
    *type*, *name* and *args* will be derived from *t*, all others are
    initialized with None. 
    """
    def __init__(self, t):
        assert isinstance(t, type.HiltiType)
        self.type = t
        self.name = t.name()
        self.c_prototype = ""
        self.args = t.args() if isinstance(t, type.Parameterizable) else []
        self.to_string = None
        self.to_int64 = None
        self.to_double = None
        self.hash = "hlt::default_hash"
        self.equal = "hlt::default_equal"
        self.aux = None
    
class CodeGen(objcache.Cache):
    """Implements the generation of LLVM code from a HILTI module."""
 
    TypeInfo = TypeInfo

    level = 0
    
    def XX__getattribute__(self,name):
        
        def _(*args, **kw):
            print >>sys.stderr, "$$", name, args, kw
            x = object.__getattribute__(self, name)
            return x(*args, **kw)
            
        x = object.__getattribute__(self, name)
        try:
            if x.__self__:
                return _
        except:
            pass
        
        return x
                
    def codegen(self, mod, libpaths, debug=0, stack=16384, trace=False, verify=True):
        """Compiles a HILTI module into a LLVM module.  The module must be
        well-formed as verified by ~~validateModule, and it must have been
        canonified by ~~canonifyModule.
        
        mod: ~~Node - The root of the module to turn into LLVM.
        
        libpaths: list of strings - List of paths to be searched for libhilti prototypes.
        
        debug: int - Debugging level. With 1 or 2, debugging support or more
        debugging support is compiled in.
        
        stack: int - The default stack segment size. 
        
        trace: bool - If true, debugging information will be printed to stderr
        tracing where code generation currently is. 

        verify: bool - If true, the correctness of the generated LLVM code will
        be verified via LLVM's internal validator.
        
        Returns: tuple (bool, llvm.core.Module) - If the bool is True, code generation (and
        if *verify* is True, also verification) was successful. If so, the second
        element of the tuple is the resulting LLVM module.
        """
        self._libpaths = libpaths
        self._debug = debug
        self._trace = trace
        self._stack_segment_size = stack
        
        # Dummy class to create a sub-namespace.
        class _llvm_data:
            pass

        self._llvm = _llvm_data()
        self._llvm.module = llvm.core.Module.new(mod.name())
        
        self._llvm.prototypes = None
        self._readPrototypes()

        self._libpaths = []        # Search path for libhilti prototypes.
        self._modules = []
        self._functions = []
        self._block = None
        self._func_counter = 0
        self._label_counter = 0
        self._global_label_counter = 0
        self._global_id_counter = 0
        self._globals = {}
        
        self._llvm.frameptr = None # Frame pointer for current LLVM function.
        self._llvm.ctxptr = None   # Execution context pointer for current LLVM function.
        self._llvm.eoss = None     # Current end-of-stack-segemtn pointer. 
        self._llvm.builders = []   # Stack of llvm.core.Builders
        self._llvm.global_ctors = [] # List of registered ctors to run a startup.
        
        mod.codegen(self)
        
        self._llvmGenerateExecutionContextFunc()
        self._llvmGenerateCtors()
        
        return self.llvmModule(verify)

    def debugLevel(self):
        """Returns the debug level we're compiling in. 0 is no debug support;
        1 and 2 are successively more.
        
        Returns: integer - The level.
        """
        return self._debug

    def defaultStackSegmentSize(self):
        """Returns the default stack segment size.
        
        Returns: int - The segment size.
        """
        return self._stack_segment_size
        
    def trace(self, msg):
        """Print msg to stderr if tracing is enabled. This is for debugging
        the code generator, and nothing will be printed if tracing isn't on.
        
        msg: string - The message to print.
        """
        if not self._trace:
            return
        
        print >>sys.stderr, "|", msg

    class _BuilderProxy():
        """Prints per instruction debugging output."""
        
        _cnt = 0
        
        def __init__(self, cg, builder):
            self._builder = builder
            self._cg = cg
            self._in_getattr = False
        
        def __getattr__(self, name):
            
            if not self._in_getattr and name != "phi": # phi must be first in block.
                self._in_getattr = True
                CodeGen._BuilderProxy._cnt += 1
                cnt = CodeGen._BuilderProxy._cnt
                self._cg.llvmDebugPrintStr("instruction %d, %s" % (cnt, name), builder=self._builder)
                self._in_getattr = False
                
            return self._builder.__getattribute__(name)
        
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
            
        if self.debugLevel() == 2:
            builder = CodeGen._BuilderProxy(self, builder)
            
        self._llvm.builders += [builder]
  
        #print >>sys.stderr, "A", 
        #array = llvm.core.Type.array(llvm.core.Type.int(8),  1000)
        #x = self.builder().malloc(array)
        #print >>sys.stderr, "B"
        
        return builder
        
    def popBuilder(self):
        """Removes the top-most LLVM builder from the builder stack."""
        self._llvm.builders = self._llvm.builders[0:-1]
    
    def builder(self):
        """Returns the top-most LLVM builder from the builder stack. Methods
        creating LLVM instructions should almost always use this builder.
        
        Returns: llvm.core.Builder: The most recently pushed builder.
        """
        return self._llvm.builders[-1]

    def currentModule(self):
        """Returns the current module. 
        
        Returns: ~~Module - The current module.
        """
        return self._modules[-1]
    
    def currentFunction(self):
        """Returns the current function. 
        
        Returns: ~~Function - The current function, or None if not inside a
        function.
        """
        return self._functions[-1] if len(self._functions) else None
    
    def currentBlock(self):
        """Returns the current block. 
        
        Returns: ~~Block - The current function.
        """
        return self._block

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

    def startModule(self, m):
        """Indicates that code generation starts for a new module.
        
        m: ~~Module - The module.
        """
        self._modules += [m]
        
    def endModule(self):
        """Indicates that code generation is finished with the current
        module."""
        self._module = self._modules[:-1]
    
    def startFunction(self, f):
        """Indicates that code generation starts for a new function.
        
        f: ~~Function - The function.
        """
        self._functions += [f]
        self._label_counter = 0

    def endFunction(self):
        """Indicates that code generation is finished with the current
        function."""
        self._functions = self._functions[:-1]
        self._llvm.frameptr = None
        self._llvm.eoss = None
        self._llvm.ctxptr = None
        
    def startBlock(self, b):
        """Indicates that code generation starts for a new block.
        
        b: ~~Block - The block.
        """
        assert self.currentFunction()

        self._block = b
        
        self._llvm.func = self.llvmFunctionForBlock(b)
        if self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            self._llvm.frameptr = self._llvm.func.args[0]
            self._llvm.eoss = self._llvm.func.args[1]
            self._llvm.ctxptr = self._llvm.func.args[2]
        else:
            self._llvm.frameptr = None 
            self._llvm.eoss = None
            self._llvm.ctxptr = None
        
        self._llvm.block = self._llvm.func.append_basic_block("")
        self.pushBuilder(self._llvm.block)
    
    def endBlock(self):
        """Indicates that code generation is finished with the current
        block."""
        self._llvm.func = None
        self._llvm.block = None
        self._block = None
        self.popBuilder()

    def startInstruction(self, i):
        self._ins = i
        
    def endInstruction(self):
        self._ins = None
        
    def llvmCurrentModule(self):
        """Returns the current LLVM module.
        
        Returns: llvm.core.Module - The LLVM module we're currently building.
        """
        return self._llvm.module

    def llvmCurrentFrameDescriptor(self):
        """Returns the current function's frame descriptor.
        
        Returns: ~~FrameDescriptor - The descriptor.
        """
        return self.makeFrameDescriptor(self._llvm.frameptr, self._llvm.eoss)
    
    def llvmCurrentFramePtr(self):
        """Returns the frame pointer for the current LLVM function. The frame
        pointer is the base pointer for all accesses to local variables,
        including function arguments). The execution model always passes the
        frame pointer into a function as its first parameter.

        If there is no current function with HILTI linkage, we
        return None. 
        
        Returns: llvm.core.Value - The current frame pointer.
        """
        return self._llvm.frameptr 
    
    def llvmCurrentExecutionContextPtr(self):
        """Returns the execution context pointer for the current LLVM
        function. The context pointer provides access to per-thread
        information, including a thread's set of global variables.  The HILTI
        execution model always passes the context pointer into a function as
        its second parameter.

        If there is no current function with HILTI linkage, we return the
        context pointer for the primary thread. 
        
        Returns: llvm.core.Value - The current context pointer.
        """
        return self._llvm.ctxptr if self._llvm.ctxptr else self._llvmGlobalExecutionContextPtr()
    
    def llvmCurrentEndOfStackSegmentPtr(self):
        """Returns a pointer to (one beyond) the end of the current
         stack stack segment. The HILTI execution model always
         passes the context pointer into a function as its second.

        If there is no current function with HILTI linkage, we
        return None. 
        
        Returns: llvm.core.Value - The current EOSS pointer. 
        """
        return self._llvm.eoss
    
    def lookupFunction(self, i):
        """Looks up the function for the given ID inside the current module.
        The method double-check that the returned value is indeed a function
        (via asserts). 
        
        id: ~~ID or string or ~~operand.ID - Either the ID to lookup,
        or string with the IDs name, or an operand refering to an ID. 
        
        Returns: ~~Function - The function with the given name, or None if
        there's no such identifier in the current module's scope.
        """
        
        if isinstance(i, str):
            i = self.currentModule().scope().lookup(i)
        
        if isinstance(i, operand.ID):
            i = i.value()
        
        if not i:
            return None
            
        assert isinstance(i, id.Function)
        func = i.function()
        
        if func:
            util.check_class(func.type(), type.Function, "lookupFunction")
            
        return func

    def lookupBlock(self, i):
        """Looks up block for the given ID inside the current function. 
        
        id: ID or ~~operand.ID - Either the ID to lookup, or an operand
        refering to an ID. 
        
        Returns: ~~Block - The block with the given name, or None if there's
        no such block in the current function's scope.
        """
        if isinstance(i, operand.ID):
            i = i.value()
            
        return self.currentFunction().lookupBlock(i.name())
    
    def lookupIDForType(self, ty):
        """Searches the current module's scope for an ID defining  a type. 
        
        ty: ~~Type - The type to search for.
        
        Returns: ~~ID - The ID if found, or None if not.
        """
        for i in self.currentModule().scope().IDs():
            if not isinstance(i, id.Type):
                continue

            if builtin_id(i.type()) == builtin_id(ty):
                return i
            
        assert False

    def llvmEnumLabel(self, label):
        """Looks up the internal value of an enum label.
        
        label: string - The name of the label in qualified form, i.e., ``a::b``.
        
        Returns: llvm.core.Constant - The LLVM value corresponding to *label*.
    
        Note: The functions will abort execution if *label* is not known.
        """
        v = self.currentModule().scope().lookup(label)
        
        if not v or not isinstance(v.type(), type.Enum):
            util.internal_error("%s is not a valid enum label" % label)
            
        label = v.value().value()
            
        return self.llvmConstInt(v.value().type().labels()[label], 8)
        
    ### Get the LLVM type for something.

    def llvmType(self, type):
        """Converts a ValueType into the LLVM type used for corresponding
        variable declarations.
        
        type: ~~ValueType - The type to converts.
        
        Returns: llvm.core.Type - The corresponding LLVM type for variable declarations.
        
        Note: This deals correctly with self-refering types.
        """ 
        key = "ty-%s" % type
        
        def _typeConvert():
            th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())
            self.setCacheEntry(key, th.type)
            lty = type.llvmType(self)
            th.type.refine(lty)
            return th.type
            
        return self.cache(key, _typeConvert)
    
    def llvmTypeInternal(self, name):
        """Returns an externally defined type. The type must be defined with
        the given name in ``libhilti.ll`` , and it will be automatically added
        to the current module. The method aborts if the type is not found. 
        
        name: string - The name of the type.
        
        Returns: ~~Type - The type as found in ``libhilti.ll``.
        """
        def _llvmTypeInternal():
            ty = self._llvm.prototypes.get_type_by_name(name)
            if not ty:
                util.internal_error("type %s not defined in libhilti.ll" % name)
                
            # Transfer it to our current module.
            self._llvm.module.add_type_name(name, ty)
            
            return ty
        
        return self.cache("type-%s" % name, _llvmTypeInternal)
    
    def llvmGlobalInternal(self, name):
        """Returns an externally defined global. The global must be defined
        with the given name in ``libhilti.ll`` , and it will be automatically
        added to the current module. The method aborts if the global is not
        found. 
        
        name: string - The name of the global.
        
        Returns: llvm.core.Value - The global.
        """
        return self.builder().load(self.llvmGlobalInternalPtr(name))

    def llvmGlobalInternalPtr(self, name):
        """Returns a pointer to an externally defined global. The global must
        be defined with the given name in ``libhilti.ll`` , and it will be
        automatically added to the current module. The method aborts if the
        global is not found. 
        
        name: string - The name of the global.
        
        Returns: llvm.core.Value - A pointer to the global.
        """
        
        def _llvmGlobalInternal():
            glob = self._llvm.prototypes.get_global_variable_named(name)
            if not glob:
                util.internal_error("global %s not defined in libhilti.ll" % name)
                
            # Transfer it to our current module.
            nglob = self._llvm.module.add_global_variable(glob.type.pointee, name)
            nglob.linkage = glob.linkage
            try:
                nglob.init = glob.init
            except AttributeError:
                pass
            
            return nglob
        
        return self.cache("global-%s" % name, _llvmGlobalInternal)
    
            
    def llvmTypeGenericPointer(self):
        """Returns the LLVM type used for representing a generic pointer.
        (\"``void *``\").
        
        Returns: llvm.core.Type.pointer - The pointer type.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_void"))
    
    def llvmTypeConstCharPointer(self):
        """Returns the LLVM type used for representing a pointer to constant
        characters. (\"``const char *``\").
        
        Returns: llvm.core.Type.pointer - The pointer type.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_cchar"))

    def _readPrototypes(self):
        """XXX"""
        prototypes = util.findFileInPaths("libhilti.ll", self._libpaths)
        if not prototypes:
            util.error("cannot find libhilti.ll")
        
        try:
            f = open(prototypes)
        except IOError, e:
            util.error("cannot read %s: %s" % (prototypes, e))
            
        try:
            self._llvm.prototypes = llvm.core.Module.from_assembly(f)
        except llvm.LLVMException, e:
            util.error("cannot import %s: %s" % (prototypes, e))
    
    ### Get the LLVM-level name for for something.
    
    def nameFunction(self, func, prefix=True, internal=False):
        """Returns the internal LLVM name for a function. The method processes
        the function's name according to HILTI's name mangling scheme.
        
        func: ~~Function - The function to generate the name for.
        
        prefix: bool - If false, the internal ``hlt_`` prefix is not prepended.
        
        internal: bool - If true, the name is generated as if the method had
        internal linkage, even if it doesn'.
        
        Returns: string - The mangled name.
        """

        try:
            (ns, name) = func.name().split("::")
        except ValueError:
            name = func.name()
        
        if func.callingConvention() == function.CallingConvention.C:
            # Don't mess with the names of C functions.
            return name
        
        if func.id().namespace():
            name = func.id().namespace() + "_" + name
        
        if func.callingConvention() == function.CallingConvention.HILTI or func.id().linkage() != id.Linkage.EXPORTED or internal:
            return "__hlt_%s" % name if prefix else name

        if func.callingConvention() == function.CallingConvention.C_HILTI:
            return name
        
        # Cannot be reached
        assert False

    def nameFrameType(self, func, prefix=True):
        """Returns the internal LLVM name for a function's frame type.
        
        Returns: string - The name.
        """
        
        name = self.nameFunction(func)
        if name.startswith("__"):
            name = name[2:]
        return "__frame_" + name
        
    def nameFunctionForBlock(self, block):
    	"""Returns the internal LLVM name for a block's function. The compiler
        turns all blocks into functions, and this method returns the name
        which the LLVM function for the given block gets.
        
        Returns: string - The name of the block's function.
        """
        
        function = block.function()
        funcname = self.nameFunction(function)
        name = block.name()
        
        first_block = function.blocks()[0].name()

        if name == first_block:
            return funcname

        if name.startswith("@"):
            name = name[1:]
        
        if name.startswith("__"):
            name = name[2:]
    
        return "%s_%s" % (funcname, name)

    def nameGlobal(self, id):
    	"""Returns the internal LLVM name for a global identifier.
        
        id: ~~id.Global - The global identifier.
        
        Returns: string - The name.
        """
        if id.namespace():
            return "%s_%s" % (id.namespace(), id.name())
        else:
            return "%s_%s" % (self.currentModule().name().lower(), id.name())
        
    def nameTypeInfo(self, type):
        """Returns a the name of the global holding the type's meta information.
        
        type: ~~HiltiType - The type. 
        
        Returns: string - The internal name of the global storing the type
        information.
        """
        
        canonified = type.name()
        for c in ["<", ">", ",", "{", "}", " ", "*", "="]:
            canonified = canonified.replace(c, "_")
            
        while canonified.find("__") >= 0:
            canonified = canonified.replace("__", "_")
            
        if canonified.endswith("_"):
            canonified = canonified[:-1]
            
        return "hlt_type_info_%s" % canonified        
        
    def nameNewLabel(self, tag=None):
        """Returns a new unique label for LLVM blocks. The label is guaranteed
        to be unique within the ~~currentFunction.
        
        tag: string - If given, tag is added to the generated name.
        
        Returns: string - The unique label.
        """ 
        
        func = self.currentFunction()
        if func:
            self._label_counter += 1
            cnt = self._label_counter
        else:
            self._global_label_counter += 1
            cnt = self._global_label_counter
        
        if tag:
            return "l%d-%s" % (cnt, tag)
        else:
            return "l%d" % cnt
        
    def nameNewGlobal(self, tag=None):
        """Returns a unique name for a new global. The name is guaranteed to
        be unique within the ~~currentModule.
        
        tag: string - If given, tag is added to the generated name.
        
        Returns: string - The unique name.
        """ 
        self._global_id_counter += 1
        
        if tag:
            return "g%d-%s" % (self._global_id_counter, tag)
        else:
            return "g%d" % self._global_id_counter
    
    def nameNewFunction(self, tag):
        """Returns a unique name for a new LLVM function. The name is
        guaranteed to unique within the ~~currentModule..
        
        tag: string - If given, tag is added to the generated name.
        
        Returns: string - The unique name.
        """ 
        self._func_counter += 1
        
        if tag:
            return "__%s_%s_f%s" % (self.currentModule().name(), tag, self._func_counter)
        else:
            return "__%s_f%s" % (self.currentModule().name(), self._func_counter)
        
    ### Add new objects to the current module.
    
    def llvmNewGlobalVar(self, i):
        """Adds a global variable to the current module. The variable is
        initialized with its specified value (or its default if not set), and
        will have internal linkage. 
      
        i: ~~id.Global - The global.
        """
        idx = len(self._globals) + 1 # Sic! First index is 1.
        self._globals[i.name()] = (i, idx)
        
    def llvmNewGlobalConst(self, name, value):
        """Adds a global constant to the current module. The constant is
        initialized with the given value, and will have only internal linkage.

        name: string - The name. It should have been created with one of the
        CodeGen's ``name*`` methods.
        
        value: llvm.core.Value - The value to initialize the constant with.
        
        Return: llvm.core.Value - A value representing the new constant.
        """
        glob = self.llvmCurrentModule().add_global_variable(value.type, name)
        glob.initializer = value
        glob.linkage = llvm.core.LINKAGE_INTERNAL
        glob.global_constant = True
        return glob
    
    def llvmNewGlobalStringConst(self, name, s): 
        """Adds a global string constant to the current module. The constant
        is initialized with the given string and null-terminated; it will have
        internal linkage and C type ``const char *``.
        
        name: string - The name. It should have been created with one of the
        CodeGen's ``name*`` methods.
        
        s: string - The string to initialize the constant with.
        
        Return: llvm.core.Value - A value representing the new constant.
        """
        s += '\0'
        bytes = [self.llvmConstInt(ord(c), 8) for c in s]
        array = llvm.core.Constant.array(llvm.core.Type.int(8), bytes)
        return self.llvmNewGlobalConst(name, array).bitcast(self.llvmTypeConstCharPointer())

    def llvmNewBlock(self, tag, llvm_func=None):
        """Creates a new LLVM block. The block gets a label that is guaranteed
        to be unique within the ~~currentFunction, and it is added to that
        function's implementation.
        
        tag: string - If given, the postfix is added to the block's
        label.
        
        llvm_func:  llvm.core.Function - The function to add the block to. If
        not set, the default is the current function's implmentation.
        
        Returns: llvm.core.Block - The new block.
        """         
        
        if not llvm_func:
            llvm_func = self._llvm.func
        
        return llvm_func.append_basic_block(self.nameNewLabel(tag))
    
    ### Type-info related.

    def typeInfo(self, t):
        """Returns the type information for a type.
        
        t: ~~Type - The type to return information for.
        
        Returns: ~~TypeInfo - The type information.
        """
        def _makeTypeInfo():
            return t.typeInfo(self)
        
        return self.cache("ti-%s" % t.name(), _makeTypeInfo)
    
    def llvmTypeTypeInfo(self):
        """Returns the LLVM type for representing a type's meta information.
        The field's of the returned struct correspond to the subset of
        ~~TypeInfo that is provided to the libhilti run-time; see
        ``libhilti/rtti.h``.
        
        Returns: llvm.core.Type - The type for the meta information.
        """
        return self.llvmTypeInternal("__hlt_type_info")
    
    def llvmTypeTypeInfoPtr(self):
        """Returns a pointer to the LLVM type for representing a type's meta
        information. See ~~llvmTypeTypeInfo for more information.
        
        Returns: llvm.core.Type - The pointer type.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_type_info"))
    
    def llvmTypeInfoPtr(self, t):
        """Returns an LLVM pointer to the type information for a type. 
        
        t: ~~Type - The type to return the information for.
        
        Returns: llvm.core.Value - The pointer to the type information.
        """
        
        def _llvmTypeInfoPtr():
            # We special case ref's here, for which we pass the type info of
            # whatever they are pointing at.
            if isinstance(t, type.Reference):
                ty = t.refType()
                if not ty:
                    ty = type.Any()
            else:
                ty = t
            
            return self._llvmAddTypeInfo(ty)
        
        return self.cache("typtr-%s" % t.name(), _llvmTypeInfoPtr)

    def _llvmAddTypeInfo(self, t):
        """Adds run-time type information for a type to the current LLVM module. 
        
        t: ~~HiltiType : The type to add type information for.
        """
        assert isinstance(t, type.HiltiType)
        
        ti = t.typeInfo(self)
        
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
                
            elif isinstance(arg, str):
                arg = self.llvmEnumLabel(arg)
                arg_types += [arg.type]
                arg_vals += [arg]
               
            else:
                util.internal_error("llvmAddTypeInfos: unsupported type parameter %s" % repr(arg))

        hlt_func_vals = []
        hlt_func_types = []
        
        for conversion in ("to_string", "to_int64", "to_double", "hash", "equal"):
            if ti.__dict__[conversion]:
                name = ti.__dict__[conversion]
                func = self.lookupFunction(name)
                if not func:
                    util.internal_error("llvmAddTypeInfos: %s is not a known function" % name)
                    
                func_val = self.llvmFunction(func)
            else:
                func_val = llvm.core.Constant.null(self.llvmTypeBasicFunctionPtr())
                
            hlt_func_vals += [func_val]
            hlt_func_types += [func_val.type]

        # Create a global with the type's name.
        name_glob = self.llvmNewGlobalStringConst(self.nameNewGlobal("type-name"), ti.name)
        name_glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY
    
        # Create the type info struct type. Note when changing this we
        # also need to adapt the definition of hlt_type_info.
        st = llvm.core.Type.struct(
            [llvm.core.Type.int(16),            # type
             llvm.core.Type.int(16),            # sizeof(type)
             llvm.core.Type.pointer(name_glob.type), # name
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
             llvm.core.Constant.sizeof(self.llvmType(ti.type)).trunc(llvm.core.Type.int(16)),     # sizeof(type)
             name_glob,                                                    # name
             llvm.core.Constant.int(llvm.core.Type.int(16), len(ti.args))] # num_params
            + [aux] + hlt_func_vals + arg_vals)

        glob = self.llvmNewGlobalConst(self.nameTypeInfo(ti.type), sv)
        glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY

        return glob    

    ### Threading related.
    
    def llvmGlobalThreadMgrPtr(self):
         """Returns a pointer to the global thread manager. The value of the
         pointer will be null if threading has not been initialized. 
         
         Returns: llvm.core.Type - The pointer of type
         ``__hlt_execution_context*``.
         """
         return self.llvmGlobalInternal("__hlt_global_thread_mgr")

    def llvmTypeVThreadID(self):
        """Returns the type used for storingi the IDs of virtual threads.
        
        Returns: llvm.core.Type - The type.
        """
        return self.llvmTypeInternal("__hlt_vthread_id")
     
    ### Helpers to construct and use LLVM constructs/objects.

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

    def llvmExtractValue(self, struct, idx):
        """Extract a specific index from an LLVM struct. 
                
        struct: llvm.core.Value - An LLVM value of type
        ``llvm.core.Type.struct`` (not a pointer; use GEP for pointers).
        
        idx: integer or llvm.core.Value - The zero-based index to extract,
        either as a constant integer, or as LLVM value of integer type. If an
        LLVM value, it must an integer of width 32. 
        
        Returns: llvm.core.Value - The value of the indexed field.
        
        Note: This function works around the fact that llvm-py at the moment
        does not support LLVM's ``extractvalue``. Once it does, we should
        change the implementation accordingly.
        
        Todo: The current implementation is awful. Really need the
        ``extractvalue`` instruction.
        """
        
        addr = self.llvmAlloca(struct.type)
        self.builder().store(struct, addr)
        
        if isinstance(idx, int):
            idx = self.llvmGEPIdx(idx)
            
        ptr = self.builder().gep(addr, [self.llvmGEPIdx(0), idx])
        return self.builder().load(ptr)
    
    def llvmInsertValue(self, obj, idx, val):
        """Inserts a value at a specific index in an LLVM struct or array.
        
        obj llvm.core.Value - An LLVM value of type ``llvm.core.Type.struct``
        or ``llvm.core.Type.array`` (not a pointer; use GEP for pointers).
        
        idx: integer or llvm.core.Value - The zero-based index to extract,
        either as a constant integer, or as LLVM value of integer type.
        
        val: llvm.core.Value - The new value for the field.
        
        Returns: llvm.core.Value - A value of the same type as *obj* with the
        specified field replaced. 

        Note: This function works around the fact that llvm-py at the moment
        does not support LLVM's ``insertvalue``. Once it does, we should
        change the implementation accordingly.
        
        Todo: The current implementation is awful. Really need the
        ``insertvalue`` instruction.
        """
        
        addr = self.llvmAlloca(obj.type)
        
        if addr.type.pointee != obj.type:
            util.internal_error("llvmInsertValue: cannot store type '%s' in pointer to type '%s'" % (obj.type, addr.type.pointee))
        
        self.builder().store(obj, addr)
        
        if isinstance(idx, int):
            idx = self.llvmGEPIdx(idx)
        
        ptr = self.builder().gep(addr, [self.llvmGEPIdx(0), idx])
        
        if ptr.type.pointee != val.type:
            util.internal_error("llvmInsertValue: cannot store type '%s' in pointer to type '%s'" % (val.type, ptr.type.pointee))
        
        self.builder().store(val, ptr)
        return self.builder().load(addr)

    def llvmSwitch(self, op, cases, result=False):
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
        string with the *fully qualified* name of an enum label; the
        corresponding *function* will be called be the code generator to emit
        the LLVM code for that case. The function will be called with one
        parameter, the *constant*; and it should use the current ~~builder to
        emit IR. Optionally, *constant* can also be a list of such constants,
        and the corresponding block will then be executed for all of the cases
        they specify; in this case, *function* will only be called for the
        first entry of the list. 
        
        *result*: bool - If true, the functions in *cases* are expected to
        return ``llvm.core.Value`` values, which must be all of the same type.
        These are then merged with an LLVM ``phi`` instruction to select the
        one from the branch being taken, and the ``phi`` result is returned by
        ``llvmSwitch``.
        
        Returns: llvm.core.Value or nothing - A value if *result* is true,
        nothin otherwise. 
        
        Note: The function is optimized for the case where *op* is constant;
        it then generates just the code for the corresponding branch. 
        """
        assert isinstance(op, str) or isinstance(op.type(), type.Integer) or isinstance(op.type(), type.Enum)
        
        if isinstance(op, str):
            i = self.currentModule().scope().lookup(op)
            if not i:
                util.internal_error("llvmSwitch: %s is not a know enum value" % op)
                
            op = operand.ID(i)
        
        if op.constant():
            # Version for a const operand.             
            c = op.constant().value()
            
            for (vals, function) in cases:
                if not isinstance(vals, list):
                    vals = [vals]
                    
                for val in vals:
                    if isinstance(val, str):
                        i = self.currentModule().scope().lookup(val)
                        if not i:
                            util.internal_error("llvmSwitch: %s is not a know enum value" % val)
                        val = i.value()

                    if val.value() == c:
                        return function(val)
                    
            util.internal_error("codegen.llvmSwitch: constant %s does not specify a known case" % c)
        
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
            returns = []
        
            for (vals, function) in cases:
                if not isinstance(vals, list):
                    vals = [vals]
                    
                block = self.llvmNewBlock("switch-%s" % str(vals[0]))
                self.pushBuilder(block)
                ret = function(vals[0])
                returns = (ret, block)
                self.builder().branch(cont)
                self.popBuilder()
                
                for val in vals:
                    
                    if isinstance(val, str):
                        val = self.currentModule().scope().lookup(val)
                        if not val:
                            util.internal_error("llvmSwitch: %s is not a know enum value" % val)
                        else:
                            if isinstance(val.type(), type.Enum):
                                # Lookup the label's value.
                                label = val.value().value()
                                assert isinstance(label, str)
                                val = val.type().labels()[label]
                                
                            else:
                                val = val.value().value().value()
                                assert isinstance(val, int)
                    
                    switch.add_case(llvm.core.Constant.int(llvmop.type, val), block)
        
            self.pushBuilder(cont) # Leave on stack.

            if result:
                assert returns
                phi = cont.phi(returns[0][0].type)
                for ret in returns:
                    phi.add_incoming(returns[ret][0], returns[ret][1])
                    
                return phi
                
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
        return self.llvmSafeCall(i, args)
    
    def llvmSafeCall(self, func, args):
        """Safe version of a n LLVM ``call`` statement. The method acts
        exactly like ``call` but it does extensive error checking on the given
        parameters first. If mistakes are found, expected and given types are
        printed to stderr.
        
        func: llvm.core.Value - A callable LLVM value.
        
        args: list of llvm.core.Value - The call's argument. 
        
        Note: With LLVM's ``call`` It's really a pain to locate the error as
        it simply abort somewhere in the C++ code when it finds a problem.
        Thus, this method should generally be instead of ``call``; it will
        save quite a bit of debugging time!        
        """
        
        assert func
        assert args != None
        assert isinstance(args, list) or isinstance(args, tuple)
        llvm.core.check_is_callable(func)

        ftype = func.type.pointee
        
        def abort(msg):
            print >>sys.stderr, ">>> llvmSafeCall: prototype of %s" % func.name
            for (i, t) in enumerate(ftype.args):
                print >>sys.stderr, ">>>   arg %d: %s" % (i, t)
            print >>sys.stderr, ">>> parameters given"
            for (i, a) in enumerate(args):
                if isinstance(a.type, llvm.core.PointerType):
                    t = "%s *" % a.type.pointee
                else:
                    t = str(a.type)
                
                print >>sys.stderr, ">>>   arg %d: %s" % (i, t)

            util.internal_error(msg)
                
        if ftype.arg_count != len(args):
            abort("LLVM func %s receives %d parameters, but got %d" % (func.name, ftype.arg_count, len(args)))
            
        for (i, f, a) in zip(range(len(args)), ftype.args, args):
            if f != a.type:
                abort("call arg %d: expected type '%s', but got '%s'" % (i, f, a.type))
                
        return self.builder().call(func, args)
    
    ### Exception-related methods.

    def llvmTypeException(self):
        """Returns the LLVM type for representing an exception.
        
        Returns: llvm.core.Type - The type of an exception.
        """
        # Make sure this type ends up in the target module too, as it's 
        # referenced by the exception type. 
        #
        # FIXME: Doesn't seem to work though??
        self.llvmTypeInternal("__hlt_exception_type")
        return self.llvmTypeInternal("__hlt_exception")

    def llvmTypeExceptionPtr(self):
        """Returns a pointer to the LLVM type for representing an exception.
        
        Returns: llvm.core.Type - The pointer tyoe.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_exception"))

    def llvmTypeExceptionType(self):
        """Returns the LLVM type representing the *type* of an HILTI
        exception. Also see ``libhilti/exceptions.h``. 
        
        Returns: llvm.core.Type - The type of an exception type.
        
        Note: Terminology is confusing here. HILTI exceptions have their own
        type objects, and the LLVM type of *those* is what's returned here. 
        """
        return self.llvmTypeInternal("__hlt_exception_type")
    
    def llvmTypeExceptionTypePtr(self):
        """Returns the LLVM type representing a pointer to a *type* of an
        exception. See ~~llvmTypeExceptionType for more information.
        
        Returns: llvm.core.Type - The pointer type.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_exception_type"))
    
    def llvmObjectForExceptionType(self, etype):
        """Return the LLVM object for a specific kind of exception.
        
        etype: ~~type.Exception - The kind of the exception. 
        
        Returns: llvm.core.Value - The LLVM value representing the type
        object.
        """
        def _llvmTypeException():
            
            assert etype.baseClass()
            
            tid = self.lookupIDForType(etype)
            
            if tid.type().internalName():
                return self.llvmGlobalInternalPtr(tid.type().internalName())
            
            name = tid.name()
            
            ename = self.nameNewGlobal("excpt-name")
            lname = self.llvmNewGlobalStringConst(ename, name)
            if etype.argType():
                tinfo = self.llvmTypeInfoPtr(etype.argType()).bitcast(self.llvmTypeTypeInfoPtr())
            else:
                tinfo = llvm.core.Constant.null(self.llvmTypeTypeInfoPtr())

            baseval = self.llvmObjectForExceptionType(etype.baseClass())
            assert baseval
            
            val = llvm.core.Constant.struct([lname, baseval, tinfo])
            
            globname = self.nameNewGlobal("excpt-type")
            glob = self.llvmNewGlobalConst(globname, val)
            
            return glob    

        if etype.isRootType():
            return self.llvmGlobalInternalPtr("hlt_exception_unspecified")
        
        return self.cache("excpt-type-%s" % builtin_id(etype), _llvmTypeException)
    
    def llvmNewException(self, type, arg, location):
        """Creates a new exception instance. The method uses the current
        :meth:`builder`.
        
	    type: type.Exception - The type of the exception to instantiate.
        
        arg: llvm.core.Value - The exception argument if exception takes one,
        or None otherwise. 
        
        location: Location - A location to associate with the exception.
        
        Returns: llvm.core.Value - The new exception object.
        """
        assert (type.argType() and arg) or (not type.argType() and not arg)
        etype = self.llvmObjectForExceptionType(type)
        return self._llvmNewException(etype, arg, location)    

    def llvmNewYieldException(self, resume, frame):
        """Creates a new ~~YieldException instance. 
        
        resume: llvm.core.Value - The continuation where to resume after the
        yield.
        
        frame: ~~FrameDescriptor - The frame descriptor to save in the
        exception object.
        
        Returns: llvm.core.Value - The new exception instance.
        """
        etype = self.llvmGlobalInternalPtr("hlt_exception_yield")
        excpt = self._llvmNewException(etype, None, None)    
        self.llvmExceptionStoreFrame(excpt, frame)
        self.llvmExceptionStoreContinuation(excpt, resume)
        return excpt
    
    def llvmConstNoException(self):
        """Returns the LLVM constant representing an unset exception. This
        constant can be written into the basic-frame's exeception field to
        indicate that no exeception has been raised.
        
        Returns: llvm.core.Constant - The constant. 
        """
        return llvm.core.Constant.null(self.llvmTypeExceptionPtr())

    def llvmRaiseException(self, exception):
        """Generates the raising of an exception. The exception itself should
        have been created with ~~llvmNewException.
        
	    exception: llvm.core.Value - A value with the exception.
        """
        assert self.currentFunction()
        self.llvmFrameSetException(exception)
        self.currentFunction().llvmHandleException(self)
        
    def llvmEscalateException(self, exception):
        """Propagates an exception to the caller of the current function. This
        function should normally not be used directly as it ignores any
        installed exception handlers. Use ~~llvmRaiseException instead.
        
	    exception: llvm.core.Value - A value with the exception.
        """
        assert self.currentFunction()
        ptr = self.llvmFrameExcptSucc()
        frame = self.llvmFrameExcptFrame()
        self.llvmFrameSetException(exception, frame)
        self.llvmTailCall(ptr, frame=frame)
        
    def llvmRaiseExceptionByName(self, exception, location, arg=None):
        """Generates the raising of an exception given by name. 
        
	    exception: string - The name of the internal global variable
        representing the exception *type* to raise. These names are defined in
        ``libhilti.ll``.
        
        location: Location - A location to associate with the exception.

        arg: llvm.core.Value - The exception argument if exception takes one,
        or None otherwise. 
        """
        etype = self.llvmGlobalInternalPtr(exception)
        excpt = self._llvmNewException(etype, arg, location)
        self.llvmRaiseException(excpt)

    def llvmExceptionStoreFrame(self, excpt, frame):        
        """Store a frame descriptor in an exception's corresponding
        field.
        
        excpt: llvm.core.Value - The exception object.
        frame: ~~FrameDescriptor - The frame descriptor to save.
        """
        zero = self.llvmGEPIdx(0)
        four = self.llvmGEPIdx(4) 
        five = self.llvmGEPIdx(5) 
        self.builder().store(frame.fptr(), self.builder().gep(excpt, [zero, four]))
        self.builder().store(frame.eoss(), self.builder().gep(excpt, [zero, five]))

    def llvmExceptionFrame(self, excpt):        
        """Returns the frame descriptor stored in an exception's
        corresponding field.
        
        excpt: llvm.core.Value - The exception object.
        
        Returns: ~~FrameDescriptor - The descriptor.
        """
        zero = self.llvmGEPIdx(0)
        four = self.llvmGEPIdx(4) 
        five = self.llvmGEPIdx(5) 
        fptr = self.builder().gep(excpt, [zero, four])
        eoss = self.builder().gep(excpt, [zero, five])
        return self.makeFrameDescriptor(self.builder().load(fptr), self.builder().load(eoss))

    def llvmExceptionContinuation(self, excpt):        
        """Returns the continuation stored in an exception's corresponding
        field.
        
        excpt: llvm.core.Value - The exception object.
        
        Returns: llvm.core.Value - The continuation object extracted.
        """
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1) 
        addr = self.builder().gep(excpt, [zero, one])
        return self.builder().load(addr)

    def llvmExceptionStoreContinuation(self, excpt, cont):
        """Store a continuation in an exception's corresponding field.
        
        excpt: llvm.core.Value - The exception object.
        cont: llvm.core.Value - The continuation to save.
        """
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1)
        self.builder().store(cont, self.builder().gep(excpt, [zero, one]))
    
    def _llvmNewException(self, etype, arg, location):
        # Instantiates a new exception object.
        if not arg:
            arg = llvm.core.Constant.null(self.llvmTypeGenericPointer())

        etype = self.builder().bitcast(etype, self.llvmTypeExceptionTypePtr())
            
        exception_new = self.llvmCFunctionInternal("__hlt_exception_new")
        arg = self.builder().bitcast(arg, self.llvmTypeGenericPointer())
        
        location = self.llvmNewGlobalStringConst(self.nameNewGlobal("location"), str(location))
        
        return self.llvmSafeCall(exception_new, [etype, arg, location])
        
    def llvmExceptionTest(self, excpt_ptr=None, abort_on_except=False):
        """Generates test whether an exception has been raised by a C-HILTI
        function. If so, the exception is either reraised in the current HILTI
        function, or program execution is aborted of *abort_on_except* is set.
        
        excpt_ptr: llvm.core.Value.pointer - Pointer as passed into the 
        C-HILTI function (so this must have type ``hlt_exception **``.). If
        not given, the current frame's exception field is checked.
        
        abort_on_except: bool - If true, program execution is aborted if an
        exception has been raised. 
        """
        if not excpt_ptr:
            excpt_ptr = self.llvmFrameExceptionAddr()
        
        excpt = self.builder().load(excpt_ptr)
        raised = self.builder().icmp(llvm.core.IPRED_NE, excpt, self.llvmConstNoException())

    	block_excpt = self.llvmNewBlock("excpt")
        block_noexcpt= self.llvmNewBlock("noexcpt")
        self.builder().cbranch(raised, block_excpt, block_noexcpt)
    
        self.pushBuilder(block_excpt)
        
        if not abort_on_except and self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            self.llvmRaiseException(excpt)
        else:
            abort = self.llvmCFunctionInternal("__hlt_exception_print_uncaught_abort")
            self.llvmSafeCall(abort, [excpt, self.llvmCurrentExecutionContextPtr()])
            self.builder().unreachable()

        self.popBuilder()
    
        # We leave this builder for subseqent code.
        self.pushBuilder(block_noexcpt)
    
    ### Continuation-related methods.
    
    def llvmTypeContinuation(self):
        """Returns the LLVM type used for representing continuations.
        
        Returns: llvm.core.Type.struct - The type for a continuation.
        """
        return self.llvmTypeInternal("__hlt_continuation")
    
    def llvmTypeContinuationPtr(self):
        """Returns the LLVM type used for representing pointers to
        continuations.
        
        Returns: llvm.core.Type.struct - The type for a continuation.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_continuation"))
    
    def llvmNewContinuation(self, succ, frame):
        """Create a new continuation object. 

        succ: llvm.core.Function - The entry point for the continuation
        frame: ~~FrameDescriptor - The frame descriptor for the continuation.
        
        Returns: llvm.core.Value - The continuation object. 
        """
        cont = self.llvmMalloc(self.llvmTypeContinuation())
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1) 
        two = self.llvmGEPIdx(2) 

        succ_addr = self.builder().gep(cont, [zero, zero])
        frame_addr = self.builder().gep(cont, [zero, one])
        eoss_addr = self.builder().gep(cont, [zero, two])

        succ = self.builder().bitcast(succ, self.llvmTypeBasicFunctionPtr())
        fptr = frame.fptr()
        eoss = frame.eoss()
        
        self.llvmAssign(succ, succ_addr)
        self.llvmAssign(fptr, frame_addr)
        self.llvmAssign(eoss, eoss_addr)
        
        return cont

    def llvmResumeContinuation(self, cont, ctx=None):
        """Transfers control to a previously captureds continuation.
        
        cont: llvm.core.Value - The continuation. 
        
        ctx: llvm.core.Value - The execution context to pass on. If not given,
        ~~llvmCurrentExecutionContextPtr is used. 
        """
        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()
        
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1) 
        two = self.llvmGEPIdx(2) 
        
        builder = self.builder()
        succ = builder.load(builder.gep(cont, [zero, zero]))
        fptr = builder.load(builder.gep(cont, [zero, one]))
        eoss = builder.load(builder.gep(cont, [zero, two]))
        
        self.llvmTailCall(succ, frame=self.makeFrameDescriptor(fptr, eoss), ctx=ctx)

    def _llvmBoundFunctionReturnFunc(self):
        # Creates an empty return function for bound functions. Note that this
        # works only for bound functions not returning any value, which is all
        # we support at the moment. 
        def makeReturnFunc():
            args = self.llvmTypeBasicFunctionPtr().pointee.args
            ft = llvm.core.Type.function(llvm.core.Type.void(), args)
            return_func = self._llvm.module.add_function(ft, "__hlt_call_bound_function_return")
            return_func.calling_convention = llvm.core.CC_C
            return_func.linkage = llvm.core.LINKAGE_LINKONCE_ODR
            return_func.args[0].name = "__frame"
            return_func.args[1].name = "__eoss"
            return_func.args[2].name = "__ctx"
            b = return_func.append_basic_block("")
            self.pushBuilder(b)
            self.builder().ret_void()
            self.popBuilder()            
            
            return return_func
            
        return self.cache("bound-function-return-func", makeReturnFunc)
        
    def llvmBindFunction(self, func, args):
        """Create a new continuation object representing a function call with
        arguments already bound. The returned object can be executed with
        ~~llvmCallBoundFunction.
        
        A bound function can be called only once.
        
        func: ~~Function - The function to call.
        args: ~~Operand - A tuple operand with the call's argument.

        Returns: llvm.core.Value - A continuation object.
        
        Note: You can't run the continuation directly; you must use
        ~~llvmCallBoundFunction.

        Todo: Currently we do not support functions with return values. Doing
        so will not change much here, but we can't as easily call the code
        from C anymore (see ~~__llvmFunctionRunClosure). Perhaps we don't need
        these, but if we do, we'll need code simialr to CStubs (and can
        probably reuse that). 
        """
        assert isinstance(func.type().resultType(), type.Void)

        # We need to allocate a new stack segment here, but we allocate one of
        # minimal size to not waste any memory before the function is actually
        # called. 
        size = self.llvmSizeOf(self.llvmTypeFunctionFrame(func))
        stack = self.llvmNewStackSegment(size)
        
        no_frame = self.makeFrameDescriptor(None, None)
        return_func = self._llvmBoundFunctionReturnFunc() # Always returns to our own function.
    
        frame = self.llvmMakeFunctionFrame(func, args, return_func, no_frame, return_func, no_frame, stack=stack)

        # We set the exception handler's frame to the frame itself so that we
        # get access to the exception field without the next to create another
        # frame. 
        self.llvmFrameSetExcptFrame(frame, frame)

        llvm_func = self.llvmFunctionForBlock(func)
        return self.llvmNewContinuation(llvm_func, frame)        

    def _llvmFunctionCallContinuationFunction(self):
        """Generates a C function for calling a continuation, as returned by
        ~~llvmBindFunction or ~llvmNewContiuation. The generated function can
        be called from C. The C function will be called 
        ``hlt_call_continuation`` and is prototyped in
        ``libhilti/continuation.h``.
        
        Note: We could hard-code this function in libhilti, but it's actually
        easier doing ot here with Python code as we already have all the
        machinery in place.
        
        Todo: This can't deal with bound functions returning values at the
        moment. See ~~llvmBindFunction.
        """
        
        def _llvmCallContinuationFunction():
            args = [self.llvmTypeContinuationPtr()]
            args += [self.llvmTypeExecutionContextPtr()]
            args += [llvm.core.Type.pointer(self.llvmTypeExceptionPtr())]
            
            ft = llvm.core.Type.function(llvm.core.Type.void(), args)
            func = self._llvm.module.add_function(ft, "hlt_call_continuation")
            func.calling_convention = llvm.core.CC_C
            func.linkage = llvm.core.LINKAGE_LINKONCE_ODR
            
            cont = func.args[0]
            excpt = func.args[1]
            ctx = func.args[2]
            cont.name = "cont"
            excpt.name = "excpt"
            ctx.name = "ctx"
            
            b = func.append_basic_block("")
            self.pushBuilder(b)
            
            # Call the continuation.
            llvm_func = self.llvmContinuationSucc(cont)
            frame = self.llvmContinuationFrame(cont)
            call = self.llvmSafeCall(llvm_func, [frame.fptr(), frame.eoss(), ctx])
            call.calling_convention = llvm.core.CC_FASTCALL
            
            # Transfer the exception to our argument. 
            self.builder().store(self.llvmFrameException(frame), excpt)
            
            self.builder().ret_void()
            
            self.popBuilder()
            
            return func
        
        return self.cache("call-continuation-function", _llvmCallContinuationFunction)
    
    def llvmCallBoundFunction(self, cont):
        """Executes a continuation representing a closure. The closure must
        have been created by ~~llvmBindFunction.

        Each bound function can be called only once.
    
        closure: llvm.core.Value - The closure continuation.
        
        Todo: This function is currently not implemented.  There is however a
        C function in libhilti for this purpose.
        """
        util.internal_error("llvmCallBoundFunction not implemented")
    
    def llvmContinuationSucc(self, cont):        
        """Returns the successor stored in an continuation. 
        
        cont: llvm.core.Value - The continuation object.
        
        Returns: llvm.core.Value - Pointer to the successor function. 
        """
        zero = self.llvmGEPIdx(0)
        addr = self.builder().gep(cont, [zero, zero])
        return self.builder().load(addr)
    
    def llvmContinuationFrame(self, cont):        
        """Returns the successor frame descriptor stored in an continuation. 
        
        cont: llvm.core.Value - The continuation object.
        
        Returns: ~~FrameDescriptor - The frame descriptor.
        """
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1)
        two = self.llvmGEPIdx(2)
        fptr = self.builder().gep(cont, [zero, one])
        eoss = self.builder().gep(cont, [zero, two])
        return self.makeFrameDescriptor(self.builder().load(fptr), self.builder().load(eoss))

    ### Memory operations and management.
    
    def llvmAlloca(self, ty, tag=""):
        """Allocates an LLVM variable on the stack. The variable can be used
        only inside the current LLVM function. That function must already have
        at least one block.
        
        ty: llvm.core.Type or ~~Type - The type of the variable to allocate.
        If it's a ~~Type (rather than an LLVM type), it's passed through
        ~~llvmType first. 
        
        Returns: llvm.core.Tyoe.pointer - A pointer to the allocated memory,
        typed correctly.
        
        Note: This works exactly like LLVM's ``alloca`` instruction yet it
        makes sure that the allocs are located in the current function's entry
        block. That is a requirement for LLVM's ``mem2reg`` optimization pass
        to find them."""

        if isinstance(ty, type.Type):
            ty = self.llvmType(ty)

        try: 
            b = self._llvm.func.get_entry_basic_block()
        except (TypeError, AttributeError):
            # FIXME: Get this when in building C stub; not sure why. Let's
            # ignore it for now.
            return self.builder().alloca(ty)

        builder = llvm.core.Builder.new(b)
        
        if b.instructions:
            # FIXME: This crashes when the block is empty. Is there a more
            # efficeint way to detection whether it's empty than getting all
            # instruction? Or better, can we fix llvm-py? Update: this is fixed
            # in llvm-py SVN. Change this once we update.
            builder.position_at_beginning(b)

        return builder.alloca(ty)

    def llvmSizeOf(self, ty, ptr=False):
        """Returns the size of an LLVM type in bytes.
        
        ty: llvm.core.Type - The type.
        
        Returns: llvm.core.Value - An integer with *ty*'s size.
        """
        # Portable sizeof, see
        # http://nondot.org/sabre/LLVMNotes/SizeOf-OffsetOf-VariableSizedStructs.txt    
        ptrty = llvm.core.Type.pointer(ty)
        null = llvm.core.Constant.null(ptrty)
        one = self.llvmGEPIdx(1)
        addr = null.gep([one])
        return addr if ptr else addr.ptrtoint(llvm.core.Type.int(64))
    
    def llvmMalloc(self, ty, cast_to=None):
        """Allocates memory for the given type. From the caller's perspective
        essentially acts like a ``builder().malloc(type)``. However, it must
        be used instead of that as will branch out the internal memory
        management, rather than to libc.
        
        type: llvm.core.Type or ~~Type - The type of the variable to allocate.
        If it's a ~~Type (rather than an LLVM type), it's passed through
        ~~llvmType first. 

        cast_to: llvm.core.Type or None - If given, the returned pointer is
        cast to this type. Per default, a pointer to *type* is returned.
        
        Returns: llvm.core.Tyoe.pointer - A pointer to the allocated memory,
        typed correctly.
        
        """
        if isinstance(ty, type.Type):
            ty = self.llvmType(ty)
            
        sizeof = self.llvmSizeOf(ty)
        mem = self.llvmCallCInternal("hlt_gc_malloc_non_atomic", [sizeof])
        
        if not cast_to:
            cast_to = llvm.core.Type.pointer(ty)
        
        return self.builder().bitcast(mem, cast_to)    
    
    def llvmLoadLocalVar(self, id, func=None, frame=None):
        """Returns the value of function's a variable. The variable can be
        either a parameter or a local variable in the function's scope.
        
        id: ~~id.Local - ID referencing the local.  func: ~~Function -
        Function the id is part of; if not given, ~~currentFunction is
        assumed.
        
        frame: ~~FrameDescriptor - The function's frame descriptor. If not
        given, ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The variable's value.
        """
        addr = self._llvmAddrLocalVar(id.name(), func, frame) 
        return self.builder().load(addr)
    
    def llvmStoreLocalVar(self, id, val, func=None, frame=None):
        """Stores a value for function's variable. The variable can be either
        a parameter or a local variable in the function's scope.
        
        id: ~~id.Local - ID referencing the local.
        
        val: llvm.core.Value - The value to store. 
        
        func: ~~Function - Function the id is part of; if not given,
        ~~currentFunction is used.
    
        frame: ~~FrameDescriptor - The function's frame descriptor. If not
        given, ~~llvmCurrentFrameDescriptor is used. 
        """
        addr = self._llvmAddrLocalVar(id.name(), func, frame)
        return self.llvmAssign(val, addr)

    def llvmLoadGlobalVar(self, id):
        """Returns the value of a global variable. The global must be located
        in the ~~currentModule.
        
        id: ~~id.Global - ID referencing the global. 
        
        Returns: llvm.core.Value - The value.
        """
        
        try:
            (i, idx) = self._globals[id.name()]
        except:
            util.internal_error("llvmLoadGlobalVar: unknown global ID %s" % id)

        ctx = self.llvmCurrentExecutionContextPtr()
        assert ctx
        ctx = self.builder().bitcast(ctx, self.llvmTypeExecutionContextFullPtr())
        
        zero = self.llvmGEPIdx(0)
        idx = self.llvmGEPIdx(idx)
        addr = self.builder().gep(ctx, [zero, idx])
        return self.builder().load(addr)
    
    def llvmStoreGlobalVar(self, id, val):
        """Stores a value for a global variable. The global must be located in
        the ~~currentModule.
        
        id: ~~id.Global - ID referencing the global. 
        
        val: llvm.core.Value - The value to store. 
        """
        try:
            (i, idx) = self._globals[id.name()]
        except:
            util.internal_error("llvmStoreGlobalVar: unknown global ID %s" % id)

        ctx = self.llvmCurrentExecutionContextPtr()
        assert ctx
        ctx = self.builder().bitcast(ctx, self.llvmTypeExecutionContextFullPtr())
        
        zero = self.llvmGEPIdx(0)
        idx = self.llvmGEPIdx(idx)
        addr = self.builder().gep(ctx, [zero, idx])
        self.llvmAssign(val, addr)

    def llvmTypeExecutionContextFull(self):
        """Returns the type for storing a thread's complete execution context.
        The returned type will start with an instance of
        ``hlt_execution_context``, followed by all globals defined via
        ~~llvmNewGlobal.
        
        Returns: llvm.core.StructType - The type.
        
        Note: This must only be called after all globals have already been
        created via ~~llvmNewGlobal.
        """
        
        def _llvmTypeExecutionContextFull():
            header = self.llvmTypeExecutionContext()
            types = [header]
            
            for (i, idx) in sorted(self._globals.values(), key=lambda x: x[1]):
                types += [self.llvmType(i.type())]
        
            ty = llvm.core.Type.struct(types)
            self.llvmCurrentModule().add_type_name("__hlt_execution_context_full", ty)
            
            return ty
        
        return self.cache("__hlt_execution_context", _llvmTypeExecutionContextFull)

    def llvmTypeExecutionContextFullPtr(self):
        """Returns the type for storing a pointer to a thread's execution
        context. See ~~llvmTypeExecutionContext for more information.
        
        Returns: llvm.core.Value.pointer - The pointer.
        """
        return llvm.core.Type.pointer(self.llvmTypeExecutionContextFull())
    
    def llvmTypeExecutionContext(self):
        """Returns the type for ``hlt_execution_context.``
        
        Returns: llvm.core.Type - The type.
        """
        return self.llvmTypeInternal("__hlt_execution_context")
    
    def llvmTypeExecutionContextPtr(self):
        """Returns the type for ``hlt_execution_context *``
        
        Returns: llvm.core.Value.pointer - The pointer.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_execution_context"))

    def llvmExecutionContextYield(self, ctx=None):
        """Returns the ``yield`` field from an execution context. 
        
        ctx: llvm.core.Value - Pointer to the execution context. If not give,
        ~~llvmCurrentExecutionContextPtr is used. 
        
        Returns: llvm.core.Value - The yield continutation.
        """
        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()
            
        yield_ = self.builder().gep(ctx, [self.llvmGEPIdx(0), self.llvmGEPIdx(2)])
        return self.builder().load(yield_)
    
    def llvmExecutionContextResume(self, ctx=None):
        """Returns the ``resume`` field from an execution context. 
        
        ctx: llvm.core.Value - Pointer to the execution context. If not give,
        ~~llvmCurrentExecutionContextPtr is used. 
        
        Returns: llvm.core.Value - The yield continutation.
        """
        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()
            
        yield_ = self.builder().gep(ctx, [self.llvmGEPIdx(0), self.llvmGEPIdx(3)])
        return self.builder().load(yield_)

    def llvmExecutionContextSetYield(self, cont, ctx=None):
        """Set an execution context's ``yield`` field to a continuation.
        
        cont: llvm.core.Value - The resume continutation to set.
        
        ctx: llvm.core.Value - Pointer to the execution context. If not give,
        ~~llvmCurrentExecutionContextPtr is used. 
        """
        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()
            
        yield_ = self.builder().gep(ctx, [self.llvmGEPIdx(0), self.llvmGEPIdx(2)])
        self.llvmAssign(cont, yield_)
    
    def llvmExecutionContextSetResume(self, cont, ctx=None):
        """Set an execution context's ``resume`` field to a continuation.
        
        cont: llvm.core.Value - The resume continutation to set.
        
        ctx: llvm.core.Value - Pointer to the execution context. If not give,
        ~~llvmCurrentExecutionContextPtr is used. 
        """
        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()
            
        resume = self.builder().gep(ctx, [self.llvmGEPIdx(0), self.llvmGEPIdx(3)])
        self.llvmAssign(cont, resume)

    def llvmExecutionContextVThreadID(self, ctx=None):
        """Returns the ``vid`` field from an execution context. 
        
        ctx: llvm.core.Value - Pointer to the execution context. If not given,
        ~~llvmCurrentExecutionContextPtr is used. 
        
        Returns: llvm.core.Value - The thread ID. 
        """
        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()
            
        vid = self.builder().gep(ctx, [self.llvmGEPIdx(0), self.llvmGEPIdx(0)])
        return self.builder().load(vid)
    
    def llvmYield(self, resume):
        """Generates code to yield execution back to caller/scheduler. 
        
        resume: llvm.core.Function - The function where to resume execution
        later. 
        """
        
        # Save where we want to resume.
        cont = self.llvmNewContinuation(resume, self.llvmCurrentFrameDescriptor())
        self.llvmExecutionContextSetResume(cont)
        
        # Transfer to scheduler, as defined in the execution context's yield
        # attribute. 
        yield_ = self.llvmExecutionContextYield()
        self.llvmResumeContinuation(yield_)
        
    def _llvmGlobalExecutionContextPtr(self):
        """Returns a pointer to the execution context for the main thread. 
        
        Returns: llvm.core.Type - The pointer of type
        ``__hlt_execution_context*``.
        """
        return self.llvmGlobalInternal("__hlt_global_execution_context")
    
    def _llvmGenerateExecutionContextFunc(self):
        """Generates a function allocating a new set of globals. The function
        will be called ``__hlt_execution_context_new`` and return a ``void *``
        pointer of newly allocated memory of sufficient size to store all
        global created so far by ~~llvmNewGlobalVar. All the globals in there
        will be initialized with their initial values. 
        
        The function also installs code for creating main thread's execution
        context at startup. Use ~~_llvmGlobalExecutionContextPtr to access that
        context. 
        
        Returns: llvm.core.StructType - The type.
        
        Todo: The mechanism won't work when having multiple compilation units
        as we'll need to merge their globals then. It's unclear what's the
        best way for doing so it, but for now we're limited to a single unit
        anyway and thus ignore the problem.
        """
        ty_full = self.llvmTypeExecutionContextFull()
        ty_ptr = self.llvmTypeExecutionContextPtr()
        
        # Create the function.
        tid = self.llvmTypeVThreadID()
        ft = llvm.core.Type.function(ty_ptr, [tid])
        func = llvm.core.Function.new(self.llvmCurrentModule(), ft, "__hlt_execution_context_new")
        func.linkage = llvm.core.LINKAGE_LINKONCE_ODR
        func.args[0].name = "vid"

        self._llvm.func = func
        self.pushBuilder(func.append_basic_block(""))

        # Make sure GC is initialized.
        self.llvmSafeCall(self.llvmCFunctionInternal("__hlt_init_gc"), [])
        
        # Allocate the memory for the globals.
        rec = self.llvmMalloc(ty_full)
        
        # Initialize the header.
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1)
        two = self.llvmGEPIdx(2)
        three = self.llvmGEPIdx(3)
        null = llvm.core.Constant.null(self.llvmTypeGenericPointer())
        
        self.llvmAssign(func.args[0], self.builder().gep(rec, [zero, zero, zero])) 
        self.llvmAssign(null, self.builder().gep(rec, [zero, zero, one])) # Thread pointer. 
        
        # Preset the yield/resume continuation to abort handlers. They will be
        # initialized by the run-time later. 
        abort = self.llvmCFunctionInternal("__hlt_abort")
        assert abort
        abort = self.llvmNewContinuation(abort, self.makeFrameDescriptor(None, None))
        
        self.llvmAssign(abort, self.builder().gep(rec, [zero, zero, two]))
        self.llvmAssign(abort, self.builder().gep(rec, [zero, zero, three]))
       
        # Initialize the globals.
        
        for (i, idx) in self._globals.values():
            self.trace("Init code for global %s" % i)
            
            if i.value():
                init = i.value().coerceTo(self, i.type()).llvmLoad(self)
            else:
                init = i.type().llvmDefault(self)
            
            idx = self.llvmGEPIdx(idx)
            addr = self.builder().gep(rec, [zero, idx])
            self.llvmAssign(init, addr)
            
        result = self.builder().bitcast(rec, ty_ptr)
        self.builder().ret(result)
        self.popBuilder()
        self._llvm.func = None
        
        # Add a ctor function to initialize the global execution context. 
        def _initContext(cg):
            ctx = self.llvmSafeCall(func, [self.llvmConstInt(-1, tid.width)])
            glob = self.llvmGlobalInternalPtr("__hlt_global_execution_context")
            cg.llvmAssign(ctx, glob)
        
        self.llvmNewGlobalCtor(_initContext)

    def llvmConstDefaultValue(self, ty):
        """Returns a type's default value for uninitialized instances. 
        
        ty: ~~ValueType - The type to determine the default value for.
        
        Returns: llvm.core.Constant - The default value. 
        """
        default = ty.llvmDefault(self)
        assert default
        llvm.core.check_is_constant(default)
        return default
        
    def llvmAssignWithDefault(self, type, addr):
        """Initializes a formerly unused address with a type's default value.
        The type must be a ValueType. 
        
        type: ~~~ValueType - The type determining the default value. 
        
        addr: llvm.core.Value - The address to initialize. 
        """
        self.llvmAssign(self.llvmConstDefaultValue(type), addr)
        
    def llvmAssign(self, value, addr):
        """Assignes a value to an address. This essentially acts like a
        ``builder().store(value, addr)``. However, it must be used instead of
        that as further logic might attached to the store operation.
        """
        util.check_class(value, llvm.core.Value, "llvmAssign")
        util.check_class(addr, llvm.core.Value, "llvmAssign")
        util.check_class(addr.type, llvm.core.PointerType, "llvmAssign")

        if addr.type.pointee != value.type:
            util.internal_error("llvmAssign: cannot store type '%s' in pointer to type '%s'" % (value.type, addr.type.pointee))
        
        self.builder().store(value, addr)
        
    def llvmStoreInTarget(self, target, val, coerce_from=None):
        """Stores a value in a target operand.
        
        target: ~~ID or ~~Instruction - The target operand, or an
        instruction which target we then use.  
        
        val: llvm.core.Value or ~~Operand - The value/operand to store. If an
        operand, we perform coercion if possible. 
        """
        if isinstance(target, instruction.Instruction):
            target = target.target()
        
        if isinstance(val, operand.Operand):
            val = val.coerceTo(self, target.type())
            val = self.llvmOp(val)
            
        if coerce_from:
           assert coerce_from.canCoerceTo(self, target.type())
           coerce_from.coerceTo(self, cg, target.type())         
        
        return target.llvmStore(self, val)    
    
    def _llvmAddrLocalVar(self, name, func=None, fptr=None):
        """Returns the address of a local variable inside a function's frame.
        The method returns an LLVM value object that can be directly used with
        subsequent LLVM load or store instructions.
        
        func: ~~Function - The function whose local variable should be
        accessed.
        
        fptr: llvm.core.* or None - The address of the function's frame.
        This must be an LLVM object that can act as the frame's address in the
        LLVM instructions that will be built to calculate the field's address.
        Often, it will be the result of :meth:`llvmCurrentFramePtr`. The frame
        can be None if the function is not of ~~CallingConvention ~~HILTI.
        
        name: string - The name of the local variable. 

        Returns: llvm.core.Value - An LLVM value with the address. 
        """

        if not func:
            func = self.currentFunction()
            
        if not fptr:
            fptr = self.llvmCurrentFrameDescriptor().fptr()

        if func.callingConvention() == function.CallingConvention.HILTI:
            
            def _makeFrameIdx():
                # The following fields correspond to the basic frame attributes.
                frame_idx = {}
                idx = 0
                for loc in func.locals():
                    frame_idx[loc.name()] = idx
                    idx += 1
                    
                return frame_idx
            
            frame_idx = self.cache("frame-%s-%s" % (func.name(), func.callingConvention()), _makeFrameIdx)

            # The locals are positioned *below* the fptr so we need to
            # adjust that first. To make sure everything is aligned correclty,
            # we do it in three steps.
            
            # 1. Get the top of the current stack segment (that's the end of our
            # frame). 
            fptr = self.builder().bitcast(fptr, self.llvmTypeBasicFramePtr())
            fptr = self.builder().gep(fptr, [self.llvmGEPIdx(1)])

            # 2. Move it back by one full function frame.
            fptr = self.builder().bitcast(fptr, self.llvmTypeFunctionFramePtr(func))
            fptr = self.builder().gep(fptr, [self.llvmGEPIdx(-1)])
            
            # 3. Now calculate the address of the local we're looking for. 
            zero = self.llvmGEPIdx(0)
            index = self.llvmGEPIdx(frame_idx[name])
            
            return self.builder().gep(fptr, [zero, index], name)
        
        else:
            # We create the locals dynamically on the stack for C funcs.
            try:
                clocals = func._clocals
            except AttributeError:
                clocals = {}
                func._clocals = clocals

            if name in clocals:
                return clocals[name]
                
            for i in func.scope().IDs():
                if i.name() == name:
                    addr = self.llvmAlloca(self.llvmType(i.type()), tag=name)
                    clocals[name] = addr
                    return addr

            util.internal_error("unknown local")    
    
            
    # Function frames.

    class _FrameDescriptor:
        def __init__(self, cg, fptr, eoss):
            """A frame descriptor stores the information that function needs
            to manage it's stack frame.
            
            fptr: llvm.core.Value - A pointer to the function's frame. The
            pointer points to the ``__hlt_bframe*`` entry of the frame, and
            thus the locals are located at negative indices from there. 
            
            eoss: llvm.core.Value - One beyond the end of the stack segment
            the frame pointer is part of.
            """
            self._fptr = fptr if fptr else llvm.core.Constant.null(cg.llvmTypeBasicFramePtr())
            self._eoss = eoss if eoss else llvm.core.Constant.null(cg.llvmTypeEndOfStackSegmentPtr())
            
        def fptr(self):
            """Returns the descriptor's frame pointer. The pointer points to
            the ``__hlt_bframe*`` entry of the frame, and thus the locals are
            located at negative indices from there. 
            
            Returns: llvm.core.Value - The pointer.
            """
            return self._fptr
        
        def eoss(self):
            """Returns the descriptor's end-of-stack pointer. 
            
            Returns: llvm.core.Value - The pointer.
            """
            return self._eoss
    
    def makeFrameDescriptor(self, fptr, eoss):
        """Creates a new frame descriptor. A frame descriptor stores the
        information that function needs to manage it's stack frame.
            
        fptr: llvm.core.Value - A pointer to the function's frame. The pointer
        points to the ``__hlt_bframe*`` entry of the frame, and thus the
        locals are located at negative indices from there. 
            
        eoss: llvm.core.Value - One beyond the end of the stack segment the
        frame pointer is part of.
        """
        return CodeGen._FrameDescriptor(self, fptr, eoss)
        
    def llvmTypeBasicFrame(self):
        """Returns the LLVM type used for representing a basic frame.
        
        Returns: llvm.core.Type - The type for a basic frame.
        """
        # Make sure this type ends up in the target module too, as
        # it's  referenced by the frame type. 
        self.llvmTypeInternal("__hlt_continuation")
        return self.llvmTypeInternal("__hlt_bframe")
    
    def llvmTypeBasicFramePtr(self):
        """Returns the LLVM type used for representing a pointer to a basic
        frame.
        
        Returns: llvm.core.Type - The pointer to a basic frame.
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_bframe"))

    def llvmTypeEndOfStackSegmentPtr(self):
        """Returns the LLVM type used for representing a pointer to the end of
        the current stack segment. 
        
        Returns: llvm.core.Type - The pointer..
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_eoss"))
    
    def llvmTypeFunctionFrame(self, func):
        """Returns the LLVM type used for representing a function's frame.
        
        func: ~~Function - The function.
        
        Returns: llvm.core.Type - The type for the function's frame.
        """
        
        def _llvmTypeFunctionFrame():
            locals = [self.llvmType(i.type()) for i in func.locals()]
            # Basic frame comes last!
            ty = llvm.core.Type.struct(locals + [self.llvmTypeBasicFrame()])
            self.llvmCurrentModule().add_type_name(self.nameFrameType(func), ty)
            return ty
            
        return self.cache("frametype-%s-%s" % (func.name(), func.callingConvention()), _llvmTypeFunctionFrame)
            
    def llvmTypeFunctionFramePtr(self, function):
        """Returns the LLVM type used for representing a pointer to a
        function's frame.
        
        func: ~~Function - The function.
        
        Returns: llvm.core.Type - The pointer to the function's frame.
        """
        return llvm.core.Type.pointer(self.llvmTypeFunctionFrame(function))

    def llvmNewStackSegment(self, size=0):
        """Allocates a new stack segment. 
        
        size: llvm.core.Value - The size of the stack segment. If not
        specificed, the default is ~~defaultStackSegmentSize.
        
        Returns: ~~FrameDescriptor - The descriptor describing the new stack,
        with the frame pointer set it is base. 
        """
        if not size:
            size = self.llvmConstInt(self.defaultStackSegmentSize(), 64)
        
        fptr = self.llvmCallCInternal("hlt_gc_malloc_non_atomic", [size])
        tmp = self.builder().ptrtoint(fptr, llvm.core.Type.int(64))
        tmp = self.builder().add(tmp, size)
        
        return self.makeFrameDescriptor(fptr, self.builder().inttoptr(tmp, self.llvmTypeGenericPointer()))
        
    def llvmMakeFunctionFrame(self, func, args, contNormFunc=None, contNormFrame=None, contExceptFunc=None, contExceptFrame=None, stack=None, llvm_func=None):
        """Instantiates a new function frame. The frame will be initialized
        with the given values. If any of the optional ones is not specified,
        their values will be taken from the current frame pointed to by
        ~~llvmCurrentFramePtr, with the exceptions of *contNormFrame* which by
        default will be set to ~~llvmCurrentFramePtr itself, and
        *contExceptFunc* which will be set to the current function's
        ~~llvmExceptionHandler.
        
         All local varibles are initalized with their defaults.
        
        func: ~~Function - The function to create a frame for. This must have
        calling convention ~~C_HILTI.
        
        args: list of ~~Operand - The arguments passed to the function when
        called with the created frame. 
        
        contNormFunc: llvm.core.Value - The successor function  for normal
        continuation. 
        
        contNormFrame: ~~FrameDescriptor - The successor frame descriptor for normal
        continuation. 
    
        contExceptFunc: llvm.core.Value - The successor function for
        exceptional continuation. 
        
        contExceptFrame: ~~FrameDescriptor - The successor frame descriptor
        for exceptional continuation. 
        
        stack: ~~FrameDescriptor - The stack segment on which to allocate the
        new frame. If not given, ~~llvmCurrentFrameDescriptor is used. 
        """
        assert func.callingConvention() == function.CallingConvention.HILTI

        if not llvm_func:
            llvm_func = self._llvm.func
            
        assert llvm_func
        
        zero = self.llvmGEPIdx(0)
        one = self.llvmGEPIdx(1)
        
        if not stack:
            stack = self.llvmCurrentFrameDescriptor()
            
        fptr = stack.fptr()
        eoss = stack.eoss()
        
        # We first need to increase the current fptr by sizeof(__hlt_bframe)
        # to get to the current end of the allocated stack space. 
        fptr = self.builder().bitcast(fptr, self.llvmTypeBasicFramePtr())
        fptr = self.builder().gep(fptr, [one])
        
        # Now we increase the frame pointer further by
        # sizeof(_hlt_frame_for_the_called_function).
        fptr = self.builder().bitcast(fptr, self.llvmTypeFunctionFramePtr(func))
        end = self.builder().gep(fptr, [one])
        end = self.builder().bitcast(end, self.llvmTypeGenericPointer())
            
        # If this end pointer is still within the bounds of our current segment,
        # we have enough space. If not, we need to allocate a new segment. 
        
        block_current = self.builder().block
        block_avail = llvm_func.append_basic_block("stack-avail")
        block_exhausted = llvm_func.append_basic_block("stack-exhausted")
        
        avail = self.builder().icmp(llvm.core.IPRED_ULT, end, eoss)
        self.builder().cbranch(avail, block_avail, block_exhausted)

        # Not enough space, allocate a new stack segment. 
        self.pushBuilder(block_exhausted)

           # Calculate min(defaultStackSegmentSize(), sizeof(frame))
        default = self.llvmConstInt(self.defaultStackSegmentSize(), 64)
        needed = self.llvmSizeOf(self.llvmTypeFunctionFrame(func))
        
        cond = self.builder().icmp(llvm.core.IPRED_SGT, needed, default)
        size = self.builder().select(cond, needed, default)
        
        new_segment = self.llvmNewStackSegment(size)
        new_fptr = self.builder().bitcast(new_segment.fptr(), self.llvmTypeFunctionFramePtr(func))
        
        self.builder().branch(block_avail)
        self.popBuilder()
        
        # Joint continuation here. 
        self.pushBuilder(block_avail)

        adj_fptr = fptr
        fptr = self.builder().phi(fptr.type)
        fptr.add_incoming(adj_fptr, block_current)
        fptr.add_incoming(new_fptr, block_exhausted)

        eoss = self.builder().phi(eoss.type)
        eoss.add_incoming(stack.eoss(), block_current)
        eoss.add_incoming(new_segment.eoss(), block_exhausted)

        # ftpr now points to sufficient space for a full frame, and eoss points
        # to the end of the corresponding segment. We now need to adjust fptr so
        # that it points to the hlt_bframe inside the full frame. 
        last = self.llvmGEPIdx(len(fptr.type.pointee.elements)-1)
        fptr = self.builder().gep(fptr, [zero, last])
            
        # Fill in defaults where not given.
        if not contNormFunc:
            contNormFunc = self.llvmFrameNormalSucc()
        else:
            contNormFunc = self.builder().bitcast(contNormFunc, self.llvmTypeBasicFunctionPtr())
            
        if not contExceptFunc:
            contExceptFunc = self.currentFunction().llvmExceptionHandler(self)
        else:
            contExceptFunc = self.builder().bitcast(contExceptFunc, self.llvmTypeBasicFunctionPtr())

        if not contNormFrame:
            contNormFrame = self.llvmCurrentFrameDescriptor()

        if not contExceptFrame:
            contExceptFrame = self.llvmFrameExcptFrame()
            
        util.check_class(contNormFrame, CodeGen._FrameDescriptor, "llvmMakeFunctionFrame")
        util.check_class(contExceptFrame, CodeGen._FrameDescriptor, "llvmMakeFunctionFrame")
        
        self.llvmAssign(contNormFunc, self._llvmAddrInBasicFrame(fptr, 0, 0))
        self.llvmAssign(contNormFrame.fptr(), self._llvmAddrInBasicFrame(fptr, 0, 1))
        self.llvmAssign(contNormFrame.eoss(), self._llvmAddrInBasicFrame(fptr, 0, 2))
        self.llvmAssign(contExceptFunc, self._llvmAddrInBasicFrame(fptr, 1, 0))
        self.llvmAssign(contExceptFrame.fptr(), self._llvmAddrInBasicFrame(fptr, 1, 1))
        self.llvmAssign(contExceptFrame.eoss(), self._llvmAddrInBasicFrame(fptr, 1, 2))
        self.llvmAssign(self.llvmConstNoException(), self._llvmAddrInBasicFrame(fptr, 2, -1))

        # Initialize function arguments. 
        ids = func.type().args()
        
        if isinstance(args, operand.Constant):
            args = args.value().value()
        
        assert len(args) == len(ids)

        for (i, arg) in zip(func.type().args(), args):
            self.llvmStoreLocalVar(i, self.llvmOp(arg, coerce_to=i.type()), frame=fptr, func=func)

        # Initialize local variables.
        for i in func.locals():
            if isinstance(i, id.Local):
                init = i.value().coerceTo(self, i.type()).llvmLoad(self) if i.value() else i.type().llvmDefault(self)
                self.llvmStoreLocalVar(i, init, frame=fptr, func=func)

        return self.makeFrameDescriptor(fptr, eoss)

    def llvmFrameNormalSucc(self, frame=None):
        """Returns the normal successor function stored in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The extracted value.
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
        
        return self.builder().load(self._llvmAddrInBasicFrame(fptr, 0, 0))
    
    def llvmFrameNormalFrame(self, frame=None):
        """Returns the normal successor frame stored in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The extracted value.
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
        
        nfptr = self.builder().load(self._llvmAddrInBasicFrame(fptr, 0, 1))
        neoss = self.builder().load(self._llvmAddrInBasicFrame(fptr, 0, 2))
        return self.makeFrameDescriptor(nfptr, neoss)
    
    def llvmFrameExcptSucc(self, frame=None):
        """Returns the exceptional successor function stored in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The extracted value.
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
            
        return self.builder().load(self._llvmAddrInBasicFrame(fptr, 1, 0))
    
    def llvmFrameExcptFrame(self, frame=None):
        """Returns the frame descriptor for the execptional successor stored
        in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The extracted value.
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
            
        nfptr = self.builder().load(self._llvmAddrInBasicFrame(fptr, 1, 1))
        neoss = self.builder().load(self._llvmAddrInBasicFrame(fptr, 1, 2))
        return self.makeFrameDescriptor(nfptr, neoss)
    
    def llvmFrameSetExcptFrame(self, frame, val):
        """Sets the frame descriptor for the exeptional successor stored in a
        frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to update. 
        
        val: ~~FrameDescriptor - The frame descriptor to write into the
        exeception handler's frame field. 
        """
        
        fptr = frame.fptr()
        self.llvmAssign(val.fptr(), self._llvmAddrInBasicFrame(fptr, 1, 1))
        self.llvmAssign(val.eoss(), self._llvmAddrInBasicFrame(fptr, 1, 2))
        
    def llvmFrameException(self, frame=None):
        """Returns the exception field stored in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The extracted value.
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
            
        return self.builder().load(self._llvmAddrInBasicFrame(fptr, 2, -1))

    def llvmFrameExceptionAddr(self, frame=None):
        """Returns the address of the exception field stored in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        
        Returns: llvm.core.Value - The extracted value.
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
            
        return self._llvmAddrInBasicFrame(fptr, 2, -1)
            
    def llvmFrameSetException(self, excpt, frame=None):
        """Sets exception field in a frame.  
        
        excpt: llvm.core.Value - The exception to set. 
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        """
        if not frame:
            fptr = self._llvm.frameptr
        else:
            fptr = frame.fptr()
            
        addr = self._llvmAddrInBasicFrame(fptr, 2, -1)
        self.llvmAssign(excpt, addr)
        
    def llvmFrameClearException(self, frame=None):
        """Clears the exception field in a frame.  
        
        frame: ~~FrameDescriptor - The frame descriptor to use. If not given,
        ~~llvmCurrentFrameDescriptor is used. 
        """
        if not frame:
            frame = self.llvmCurrentFrameDescriptor()
            
        self.llvmFrameSetException(self.llvmConstNoException(), frame)
    
    def _llvmAddrInBasicFrame(self, frame, index1, index2, cast_to=None, tag=""):
        # Helper to get a field in a basic frame struct.
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
            addr = self.builder().gep(frame, index, tag)
            return self.builder().bitcast(addr, cast_to, tag)    

    ### Functions. 

    def llvmTypeBasicFunctionPtr(self):
        """Returns the generic LLVM type used for HILTI functions. This is the
        generic version for pointers that need to point functions with
        different signatures; it's essentially ``void f(hlt_bframe*)``.  
        
        Returns: llvm.core.Type.pointer - The function pointer type. 
        """
        return llvm.core.Type.pointer(self.llvmTypeInternal("__hlt_func"))
        
    def llvmCFunctionInternal(self, name):
        """Returns an externally defined C function. The function must be
        defined with the given name in ``libhilti.ll`` , and it will be
        automatically added to the current module. The method aborts if the
        type is not found. The function's signature it not modified in any
        way. 
        
        name: string - The name of the type.
        
        Returns: llvm.core.value - The function as found in ``libhilti.ll``.
        """
        def _llvmGetCFunctionInternal():
            func = self._llvm.prototypes.get_function_named(name)
            self._llvm.module.add_function(func.type.pointee, name)
            return self._llvm.module.get_function_named(name)

        return self.cache("internal-func-%s" % name, _llvmGetCFunctionInternal)

    def llvmCallCInternal(self, name, args):
        """Calls an externally defined C function. The function must be
        defined with the given name in ``libhilti.ll`` , and it will be
        automatically added to the current module. The method aborts if the
        type is not found. Neither the function's signature nor the arguments
        are modified in any way. 
        
        name: string - The name of the type.
        
        args: list of llvm.core.Value - The arguments for the call.
        
        Returns: llvm.core.value - The function's return value, if any.
        """
        func = self.llvmCFunctionInternal(name)
        return self.llvmSafeCall(func, args)
    
    def llvmFunction(self, func):
        """Returns the LLVM function for a function or a block. The LLVM
        function is constructed according to the HILTI calling conventions,
        and it will be added to the ~~currentModule. If the method is called
        subsequently with the same function, the previously created function
        is returned.
        
        func: ~~Function or ~~Block - The function, or the block for which the
        corresponding LLVM function is wanted.
        
        Returns: llvm.core.Value - The LLVM function.
        """
        def _llvmFunction():
            if isinstance(func, block.Block):
                b = func
                f = b.function()
                is_first = (builtin_id(b) == builtin_id(f.blocks()[0]))
                name = self.nameFunctionForBlock(b)
            else:
                b = func.blocks()[0] if func.blocks() else None
                f = func
                is_first = True
                name = self.nameFunctionForBlock(b) if b else self.nameFunction(f)

            if f.callingConvention() == function.CallingConvention.HILTI:
                # Build the signature according to the HILTI calling convention.
                rtype = llvm.core.Type.void()
                
                if f.callingConvention() == function.CallingConvention.HILTI:
                    args = self.llvmTypeBasicFunctionPtr().pointee.args
                else:
                    args = []
                    
                ft = llvm.core.Type.function(rtype, args)
                llvm_func = self._llvm.module.add_function(ft, name)
                llvm_func.linkage = llvm.core.LINKAGE_LINKONCE_ANY
                llvm_func.calling_convention = llvm.core.CC_FASTCALL
        
                if f.id().linkage() != id.Linkage.EXPORTED or not is_first:
                    llvm_func.linkage = llvm.core.LINKAGE_INTERNAL
            
                # Set parameter names. 
                if f.callingConvention() == function.CallingConvention.HILTI:
                    llvm_func.args[0].name = "__frame"
                    llvm_func.args[1].name = "__eoss"
                    llvm_func.args[2].name = "__ctx"

                # Done for HILTI functions.
                return llvm_func
            
            elif f.callingConvention() == function.CallingConvention.C:
                # Don't mess with the types. 
                args = [(arg.name(), self.llvmType(arg.type())) for arg in f.type().args()]
                
            elif f.callingConvention() == function.CallingConvention.C_HILTI:
                # Convert the arguments and result type according to C-HILTI convention.
                args = []
                for arg in f.type().args():
                    ty = arg.type()
                    if (isinstance(ty, type.HiltiType) and ty.cPassTypeInfo()):
                        # Add a type information parameter.
                        args += [("__ti_%s" % arg.name(), self.llvmTypeTypeInfoPtr())]
                        # Turn argument into a pointer. 
                        args += [(arg.name(), self.llvmTypeGenericPointer())]
                    else:
                        args += [(arg.name(), self.llvmType(ty))]
                    
                args += [("__excpt", llvm.core.Type.pointer(self.llvmTypeExceptionPtr()))]
                args += [("__ctx", self.llvmTypeExecutionContextPtr())]
                
            else:
                util.internal_error("unknown calling convention %s" % cc)

            # The following is the same for C and C_HILTI.
            
            rtype = self.llvmType(f.type().resultType())
            
            # Depending on the plaform ABI, structs may or may not be returned
            # by value. If not, there's a hidden first parameter passing a
            # pointer into the function to store the value in, instead of the
            # actual return value.
            hidden_return_arg = False
            if isinstance(rtype, llvm.core.StructType):
                abi_rtype = system.returnStructByValue(rtype)
                if not abi_rtype:
                    # Need an additional argument.
                    args = [("agg.tmp", llvm.core.Type.pointer(rtype))] + args
                    rtype = llvm.core.Type.void()
                    hidden_return_arg = True
                else:
                    rtype = abi_rtype
                
            ft = llvm.core.Type.function(rtype, [arg[1] for arg in args])
            llvm_func = self._llvm.module.add_function(ft, name)
            llvm_func.calling_convention = llvm.core.CC_C
            
            if f.id().linkage() != id.Linkage.EXPORTED:
                llvm_func.linkage = llvm.core.LINKAGE_INTERNAL
        
            # Set parameter names. 
            for i in range(len(args)):
                (name, ty) = args[i]
                llvm_func.args[i].name = name
            
            if hidden_return_arg:
                llvm_func.args[0].add_attribute(llvm.core.ATTR_NO_ALIAS)
                llvm_func.args[0].add_attribute(llvm.core.ATTR_STRUCT_RET)

            return llvm_func
            
        util.check_class(func, [block.Block, function.Function], "llvmFunction")
        
        if isinstance(func, block.Block):
            name = self.nameFunctionForBlock(func)
            cc = func.function().callingConvention()
        else:
            name = self.nameFunctionForBlock(func.blocks()[0]) if func.blocks() else self.nameFunction(func)
            cc = func.callingConvention()

        return self.cache("func-%s-%s" % (name, cc), _llvmFunction)
        
    def llvmFunctionForBlock(self, block):
        """Returns the LLVM function for a block. The code generator
        internally translate each block into its own function. The block
        function is constructed according to the HILTI calling conventions,
        and it will be added to the ~~currentModule. If the method is called
        subsequently with the same block, the previously created function is
        returned.
        
        block: ~~Block - The block.
        
        Returns: llvm.core.Value - The LLVM function.
        """
        return self.llvmFunction(block)

    def llvmCallCWithRetry(self, succ, call_func, result_func=None):
        """Calls a C-HILTI function that can potentially block. If the
        function wants to block (as signaled by raising an ~~WouldBlock
        exception), execution is yielded back to the scheduler. When resuming,
        the same call is executed again, and this process repeast until no
        blocking is indicated. 
        
        succ: ~~Block - The block where execution is to continue in the

        call_func: Python function - Callback function that must return a
        tuple ``(func, args)``. *func* is ~Function, string, or
        llvm.core.Value being either a ~~Function to call; or the name of the
        function to call; or an LLVM function previously returned by
        ~~llvmFunction. *args* is a list of ~~Operand, or list of tuples
        (llvm.core.Value, ~~Type) with the arguments for the call. The
        automatically coerced types to what the function's signature specifies
        where possible. 
        
        result_func: Python function - If the call is successful (i.e.,
        doesn't block and doesn't throw any exceptions), this function will be
        called. It receives the current ~~CodeGen as its first argument, and
        and the function's return value in the form of an ``llvm.core.Value``
        as its second arguments.
        """
        # We need to create a new function here that does the C call. That
        # functiom will then be the destination for a resume continuation. 
        fname = self.nameFunctionForBlock(self.currentBlock()) + "_retry"
        ffunc = self.llvmNewHILTIFunction(self.currentFunction(), fname)
        
        self.llvmTailCall(ffunc)
        
        fblock = ffunc.append_basic_block("")
        self.pushBuilder(fblock)
        
        # TODO: This is clearly a hack. Need to build a better interface
        # for building a function while another is in progress.
        old_func = self._llvm.func
        old_frameptr = self._llvm.frameptr
        old_eoss = self._llvm.eoss
        old_ctxptr = self._llvm.ctxptr
        
        self._llvm.func = ffunc
        self._llvm.frameptr = ffunc.args[0]
        self._llvm.eoss = ffunc.args[1]
        self._llvm.ctxptr = ffunc.args[2]
            
        def _excpt_test(cg, excpt):
            # First check for a WouldBlock exception. 
            
            would_block = cg.llvmGlobalInternalPtr("hlt_exception_would_block")
            cmp = cg.llvmCallCInternal("__hlt_exception_match_type", [cg.builder().load(excpt), would_block])
            
            yes = cg.llvmNewBlock("yield")
            no = cg.llvmNewBlock("no-yield")
            cg.builder().cbranch(cmp, yes, no)
            
            cg.pushBuilder(yes)
            self.llvmFrameClearException()
            cg.llvmYield(ffunc)
            cg.popBuilder()
            
            cg.pushBuilder(no)
            # Standard exception test.
            cg.llvmExceptionTest(excpt)
            # Leave builder on stack. 
            
        (cfunc, args) = call_func(self)
        result = self.llvmCallC(cfunc, args, excpt_test=_excpt_test)
            
        if result_func != None:
            # Process return value.
            result_func(self, result)
        
        fdesc = self.makeFrameDescriptor(ffunc.args[0], ffunc.args[1])
        self.llvmTailCall(succ, frame=fdesc, ctx=ffunc.args[2])
        self.builder().ret_void()
        
        self._llvm.func = old_func
        self._llvm.frameptr = old_frameptr
        self._llvm.eoss = old_eoss
        self._llvm.ctxptr = old_ctxptr
        
    def llvmCallC(self, func, args=[], prototype=None, abort_on_except=False, excpt_test=None):
        """Calls a function with C/C_HILTI linkage. This method cannot be used
        for calling HILTI functions; use ~~llvmTailCall instead. 
        
        func: ~Function, string, or llvm.core.Value - Either the ~~Function to
        call; or the name of the function to call; or an LLVM function
        previously returned by ~~llvmFunction. In the latter case *prototype*
        must be given.
        
        args: list of ~~Operand, or list of tuples (llvm.core.Value, ~~Type) -
        The arguments for the call. The automatically coerced types to what
        the function's signature specifies where possible. 
        
        prototype: ~~Function - The function being called if ~~func is an
        ``llvm.core.value``. This is so that we know the function's signature
        in any case.
        
        abort_on_except: bool - If true, execution will be aborted if the
        called function raises an exception. If this is set, the method
        doesn't need a current framepointer and can be used from non-HILTI
        functions. 
        
        excpt_test: Python function - A Python function that is called instead
        of doing the standard exception test. The function receives two
        arguments: the current ~~CodeGen, and an ``llvm.core.Value`` with the
        current exception value as set by the called function (which will be
        null if no exception has been raised). 
        """
        llvm_func = None

        # Get the LLVM function.
        if isinstance(func, function.Function):
            llvm_func = self.llvmFunction(func)
            prototype = func
            
        elif isinstance(func, str):
            name = func
            func = self.lookupFunction(name)
            if not func:
                util.internal_error("unknown function %s" % name)
            llvm_func = self.llvmFunction(func)
            prototype = func
                
        else:
            assert prototype
            llvm_func = func

        assert isinstance(llvm_func, llvm.core.Value)

        if isinstance(args, operand.Constant) and isinstance(args.type(), type.Tuple):
            args = args.value().value()

        ops = self._llvmCallArgsToOps(args, prototype)

        cc = prototype.callingConvention()
        
        excpt = None
        
        if cc == function.CallingConvention.C_HILTI:
            # Turn the arguments into the format used by C-HILTI. 
            llvm_args = []
            for (op, arg) in zip(ops, func.type().args()):
                llvm_op = self.llvmOp(op)
                if (isinstance(arg.type(), type.HiltiType) and arg.type().cPassTypeInfo()):
                    # Add a type information parameter.
                    llvm_args += [self.builder().bitcast(self.llvmTypeInfoPtr(op.type()), self.llvmTypeTypeInfoPtr())]
                    # Turn argument into a pointer.
                    tmp = self.llvmAlloca(llvm_op.type)
                    self.builder().store(llvm_op, tmp)
                    tmp = self.builder().bitcast(tmp, self.llvmTypeGenericPointer())
                    llvm_args += [tmp]
                    
                else:
                    llvm_args += [llvm_op]
                
            # Add exception argument.
            if not abort_on_except and self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
                # If we are inside a HILTI function, we use the frame's
                # exception field.
                excpt = self.llvmFrameExceptionAddr()
            else:
                # We don't have a function frame so create a local to store the
                # exception information.
                excpt = self.llvmAlloca(self.llvmTypeExceptionPtr())
                self.llvmAssign(self.llvmConstNoException(), excpt)

            # Add context argument.
            ctx = self.llvmCurrentExecutionContextPtr()
                
            llvm_args = llvm_args + [excpt, ctx]
            
        elif cc == function.CallingConvention.C:
            # Don't mess further with the arguments. 
            llvm_args = [self.llvmOp(op) for op in ops]
            excpt = None
        
        elif cc == function.CallingConvention.HILTI:
            util.internal_error("Must not call llvmCallCFunction with function of cc HILTI")
        
        else:
            util.internal_error("unknown calling convention %s" % cc)

        # Need to handle returned struct per the ABI definition.
        llvm_rtype = self.llvmType(prototype.type().resultType())
        abi_rtype = llvm_func.type.pointee.return_type
        hidden_return_arg = None
        
        # isistancne doesn't work void, as there's no class ...
        if isinstance(llvm_rtype, llvm.core.StructType) and abi_rtype == llvm.core.Type.void():
            # Result passed via additional hidden argument.
            hidden_return_arg = self.llvmAlloca(llvm_rtype, "agg.tmp")
            llvm_args = [hidden_return_arg] + llvm_args
            
        # Do the call.
        call = self.llvmSafeCall(llvm_func, llvm_args)
        call.calling_convention = llvm.core.CC_C
        
        if excpt:
            if excpt_test != None:
                excpt_test(self, excpt)
            else:
                self.llvmExceptionTest(excpt, abort_on_except)

        if hidden_return_arg:
            # Return value is in additional argument. 
            result = self.builder().load(hidden_return_arg)
            
        elif isinstance(llvm_rtype, llvm.core.StructType):
                # Need to cast the return value from the ABI's integer.
                #
                # FIXME: We can't cast an integer into a struct, so we do
                # it manually. However, this doesn't get optimized away
                # unfortunately.
                tmp = self.llvmAlloca(llvm_rtype)
                tmp_casted = self.builder().bitcast(tmp, llvm.core.Type.pointer(call.type))
                self.builder().store(call, tmp_casted)
                result = self.builder().load(tmp)
    
        else:
            result = call
                
        return result
    
    def llvmTailCall(self, func, frame=None, ctx=None, addl_args=[], prototype=None):
        """Calls a function with HILTI linkage. The call will be tail call,
        i.e., any subsequent code in the same block will be never be reached. 
        This method cannot be used for calling HILTI functions; use
        ~~llvmCallC instead. 
        
        func: ~Function or ~~Block or llvm.core.Value - Either the ~~Function
        to call; or the ~~Block whose function will be called; or
        an LLVM function previously returned by ~~llvmFunction,
        ~~llvmFunctionForBlock, or ~~llvmNewHILTIFunction. 
        
        frame: ~~FrameDescriptor - The frame descriptor to pass to the called
        function. If not given, ~~llvmCurrentFrameDescriptor is used. 
        
        ctx: llvm.core.value - The execution context pointer to pass to the
        called function. If not given, ~~llvmCurrentContextPtr is used. 
        
        addl_args: list of ~~Operand, or list of tuples (llvm.core.Value,
        ~~Type) - Additional arguments for the call, which will be appended to
        any generated for the HILTI calling convention. Note that these are
        *not* the arguments *func* defines, as they are passed via the frame.
        Note that there isn't any coercion performed for these arguments. 
        """
        
        # Turn all additional arguments into LLVM values.
        ops = self._llvmCallArgsToOps(addl_args, None)
        llvm_addl_args = [self.llvmOp(op) for op in ops]
        
        llvm_func = None
        
        if not frame:
            frame = self.llvmCurrentFrameDescriptor()

        if not ctx:
            ctx = self.llvmCurrentExecutionContextPtr()

        util.check_class(frame, CodeGen._FrameDescriptor, "llvmTailCall")
            
        # Get the LLVM function.
        if isinstance(func, function.Function):
            assert func.callingConvention() == function.CallingConvention.HILTI
            llvm_func = self.llvmFunction(func)
        
        elif isinstance(func, block.Block):
            llvm_func = self.llvmFunctionForBlock(func)
            
        else:
            # If we get the raw LLVM function, in general we can't tell it's
            # precise type, because it may be just a __hlt_func*, with with the
            # generic basic_frame signature. So, in this case, we simply treat
            # it as such, casting it the type we'd expect it to have (including
            # any addl arguments we got passed). 
            addl_types = [arg.type for arg in llvm_addl_args]
            voidt = llvm.core.Type.void()
            
            rt = self.llvmTypeBasicFunctionPtr().pointee.return_type
            args = self.llvmTypeBasicFunctionPtr().pointee.args
            
            ft = llvm.core.Type.function(rt, args + addl_types)
            llvm_func = self.builder().bitcast(func, llvm.core.Type.pointer(ft))
            
        assert isinstance(llvm_func, llvm.core.Value)
        
        # Cast the frame pointer to right type.
        if self.currentFunction().callingConvention() == function.CallingConvention.HILTI:
            assert frame
            llvm_args = [frame.fptr(), frame.eoss(), ctx] + llvm_addl_args
        else:
            # We are in a function without frame.
            assert self.currentFunction().callingConvention() != function.CallingConvention.C_HILTI
            llvm_args = llvm_addl_args
        
        # Do the tail call.
        
            #    From the LLVM 1.5 release notes:
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
            
        call = self.llvmSafeCall(llvm_func, llvm_args)
        call.calling_convention = llvm.core.CC_FASTCALL
        call.tail_call = True
        self.builder().ret_void() # FIXME: necessary?
        
        # Any subsequent code generated inside the same block will
        # be  unreachable. We create a dummy builder to absorb it; the code will
        # not appear anywhere in the output.
        self.popBuilder();
        self.pushBuilder(None)

    def llvmNewHILTIFunction(self, hilti_func, name = None, addl_args = []):
        """Creates a new LLVM function with HILTI calling conventions.. The
        new function's signatures is built from the given HILTI function,
        according to the HILTI calling conventions. The function is added to
        :meth:`llvmCurrentModule`.
        
        hilti_func: ~~Function - The HILTI function determining the signature. 
        
        name: string - If given, the name of the new function. If not given,
        the result of ~~nameFunction is used. 
        
        addl_args: list of (name, type) - Additional arguments to add to the
        new the function's argument list. *name* is a ``string`` with the
        argument's name, and *type* is a ``llvm.core.Type`` with the
        argument's type. Note that these are arguments that appear in the
        *generated* function's signatures; it's not related to what
        *hilti_func* defines as its arguments will be passed via a frame. 
        
        Returns: llvm.core.Function - The new LLVM function. 
        """
        
        # FIXME: Is this function really only called for HILTI functions?
        assert hilti_func.callingConvention() == function.CallingConvention.HILTI

        if not name:
            name = self.nameFunction(hilti_func)
        
        # Build function type.
        if hilti_func.callingConvention() == function.CallingConvention.HILTI:
            args = self.llvmTypeBasicFunctionPtr().pointee.args
        else:
            args = []
            
        ft = llvm.core.Type.function(llvm.core.Type.void(), args + [t for (n, t) in addl_args]) 
        
        # Create function.
        func = self._llvm.module.add_function(ft, name)
        func.calling_convention = llvm.core.CC_FASTCALL
        
        if hilti_func.id().linkage() != id.Linkage.EXPORTED:
            func.linkage = llvm.core.LINKAGE_INTERNAL
        
        # Set parameter names. 
        
        if args:
            func.args[0].name = "__frame"
            func.args[1].name = "__eoss"
            func.args[2].name = "__ctx"
            
        for i in range(len(addl_args)):
            func.args[i+3].name = addl_args[i][0]

        return func
        
    def _llvmCallArgsToOps(self, args, prototype):
        # Args -> Operand with coercion.
        if prototype:
            ptypes = prototype.type().args()
        else:
            ptypes = [None] * len(args)
        
        nargs = []
        for (arg, dst) in zip(args, [p.type() if p else None for p in ptypes]):
            if not isinstance(arg, operand.Operand):
                (val, ty) = arg
                assert isinstance(val, llvm.core.Value)
                assert isinstance(ty, type.Type) 
                arg = operand.LLVM(val, ty)
            
            if dst:
                nargs += [arg.coerceTo(self, dst)]
            else:
                nargs += [arg]
            
        return nargs

        # return [self.llvmOp(arg, coerce_to=(dst.type() if dst else None)) for (arg, dst) in zip(args, ptypes)]

    ### C stub code generator.
        
    def llvmCStubs(self, func):
        """Returns the C stubs for a function. The stub functions will be 
        externally visible functions using standard C calling conventions, and
        expecting parameters corresponding to the function's HILTI parameters,
        per the usual C<->HILTI translation. The primary stub function gets
        one additional parameter of type ``hlt_exception *``, which the callee
        can set to an exception object to signal that an exception was thrown.
        The secondary function is for resuming after a previous ~~Yield; 
        it gets two parameters, the ~~YieldException and a ``hlt_exception *``,
        with the same semantics for the latter as for the primary function.
        
        All the primary stub does when called is in turn calling the HILTI
        function, adapting the calling conventions. 
        
        Note that this function just returns prototypes for the stub
        functions, while ~~llvmGenerateCStubs creates the functions
        themselves. The latter must be called only once, this method can be
        called whenever access to the stubs is needed.
                
        func: ~~Function - The function to return a stub for. The function
        must have ~~HILTI calling convention.
        
        Returns: 4-tuple of (function.Function, llvm.core.Value.Function,
        function.Function, llvm.core.Value.Function) - The first two elements
        define the primary stub function (with its HILTI and LLVM function,
        respectively), and the latter two elements likewise define the resume
        function.
        
        Note: ``hiltic`` can generate the right prototypes for such stub
        functions.
        """
    
        def _makeStubPrototypes():
            
            ### The primary stub function.
            
            # We recreate the function with linkage C-HILTI.
            cfunc = function.Function(func.type(), func.id(), func.scope(), cc=function.CallingConvention.C_HILTI, location=func.location())
            stub_func = self.llvmFunction(cfunc)
            stub_func.linkage = llvm.core.LINKAGE_LINKONCE_ANY

            ### The resume function. 
            
            rtype = cfunc.type().resultType()
            args = [id.Parameter("yield", type.Exception())]
            ft = type.Function(args, rtype)
            rid = id.Global(cfunc.name() + "_resume", ft, None, namespace=cfunc.id().namespace(), linkage=id.Linkage.EXPORTED)
            rfunc = function.Function(ft, rid, None, cc=function.CallingConvention.C_HILTI)
            resume_func = self.llvmFunction(rfunc)
            resume_func.linkage = llvm.core.LINKAGE_LINKONCE_ANY

            return (cfunc, stub_func, rfunc, resume_func)
            
        return self.cache("c-stub-%s-%s" % (func.name(), func.callingConvention()), _makeStubPrototypes )

    def llvmGenerateCStubs(self, func, resume=False):
        """Generate C stubs for a function. See ~~llvmCStubs for more
        information about stubs.
        
        func: ~~Function - The function to return a stub for. The function
        must have ~~HILTI calling convention.
        
        resume: TODO: Still used? 
        
        Todo: This function needs a refactoring. 
        """
        
        def _makeEpilog(llvm_func, dummyfunc, frame, result_id):
            # Helper function with stuff common for stub and resume. 
        
            # When we return here, the exception parameter in our frame will
            # be whatever the callee raised.  Save it in our caller's
            # argument. 
            excpt = self.llvmFrameException(frame) 
            self.builder().store(excpt, llvm_func.args[-2])
            
            # If an exception was raised, save our frame in there in case we
            # want to resume later.            
            raised = self.builder().icmp(llvm.core.IPRED_NE, excpt, self.llvmConstNoException())
            block_excpt = llvm_func.append_basic_block(self.nameNewLabel("excpt"))
            block_noexcpt = llvm_func.append_basic_block(self.nameNewLabel("noexcpt"))
            self.builder().cbranch(raised, block_excpt, block_noexcpt)

            self.pushBuilder(block_excpt)
            self.llvmExceptionStoreFrame(excpt, frame)
            self.builder().branch(block_noexcpt)
            self.popBuilder()

            self.pushBuilder(block_noexcpt)
            
            # Load and return result, if any. 
            if result_id:
                result = self.llvmLoadLocalVar(result_id, func=dummyfunc, frame=frame.fptr())
                self.builder().ret(result)
            else:
                self.builder().ret_void()

            self.popBuilder()
                
        assert func.callingConvention() == function.CallingConvention.HILTI

        
        (cfunc, stub_func, rfunc, resume_func) = self.llvmCStubs(func)
        
        # Create an local for the result, if we have one.
        rtype = func.type().resultType()
        if not isinstance(rtype, type.Void):
            result_id = id.Local("__result", func.type().resultType())
        else:
            result_id = None

        ### Create the primary stub function.
            
        # We just fake a function here that receives no arguments, and has one
        # local variable if we have to return a value. This dummy function
        # then allows us to create and use a corresponding frame. 
        ftype = type.Function([], type.Void())
        did = id.Global("%s_stub" % func.name(), ftype, None)
        dummyfunc = function.Function(ftype, did, None)
        if result_id:
            dummyfunc.scope().add(result_id)
                        
        # Create the return function for the stub, matching the format
        # "call.tail.{result,void}" expect for return functions. This body of
        # the function takes the result (handed in as additional argument) and
        # stores it in the stub's frame local variable. 
        
        if result_id:
            stub_return_addl_args = [("__result", self.llvmType(func.type().resultType()))]
        else:
            stub_return_addl_args = []
        
        stub_return_name = self.nameFunction(cfunc, internal=True) + "_return"
        stub_return_func = self.llvmNewHILTIFunction(dummyfunc, name=stub_return_name, addl_args=stub_return_addl_args)
        stub_return_block = stub_return_func.append_basic_block("")
        self.pushBuilder(stub_return_block)

        if result_id:
            self.llvmStoreLocalVar(result_id, stub_return_func.args[3], func=dummyfunc, frame=stub_return_func.args[0])

        self.builder().ret_void()
        self.popBuilder()

        # Create the yield & exception handler.
        stub_excpt_name = self.nameFunction(cfunc, internal=True) + "_excpt"
        stub_excpt_func = self.llvmNewHILTIFunction(dummyfunc, name=stub_excpt_name)
        stub_excpt_block = stub_excpt_func.append_basic_block("")
        self.pushBuilder(stub_excpt_block)
        
            # If the exception field has not been set, it's a yield. We then
            # create a yield_exception to pass back to the caller. 
        frame = self.makeFrameDescriptor(stub_excpt_func.args[0], stub_excpt_func.args[1])
        excpt = self.llvmFrameException(frame)
        
        block_yield = stub_excpt_func.append_basic_block(self.nameNewLabel("yield"))
        block_done = stub_excpt_func.append_basic_block(self.nameNewLabel("done"))
        
        raised = self.builder().icmp(llvm.core.IPRED_NE, excpt, self.llvmConstNoException())
        self.builder().cbranch(raised, block_done, block_yield)
            
        self.pushBuilder(block_yield)
        
        resume = self.llvmExecutionContextResume(ctx=stub_excpt_func.args[2])
        yexcpt = self.llvmNewYieldException(resume, frame)
        self.llvmFrameSetException(yexcpt, frame)
        self.builder().branch(block_done)
        
        self.popBuilder()

            # All done, just return.
        self.pushBuilder(block_done)
        self.builder().ret_void()
        self.popBuilder()        
        
        self.popBuilder()

        # Create the stub function itself.
        block = stub_func.append_basic_block("")
        self.pushBuilder(block)
        
            # Create the initial stack segment.
        stack = self.llvmNewStackSegment()

            # Create a frame for the stub function that it can pass into the
            # callee for continuation. We set both the normal and exception
            # frame to null as we won't need them.
        null = self.makeFrameDescriptor(None, None)
        frame = self.llvmMakeFunctionFrame(dummyfunc, [], stub_return_func, null, stub_excpt_func, null, llvm_func=stub_func, stack=stack)

            # Set the execution context's yield handler. 
        ctx = self._llvmGlobalExecutionContextPtr()
        yield_ = self.llvmNewContinuation(stub_excpt_func, frame)
        self.llvmExecutionContextSetYield(yield_, ctx)
        
            # Do the call.
            
            # FIXME: This can't deal with paramters that have type
            # information; it will mess things up ...
            
        callee_args = [operand.LLVM(arg, i.type()) for (arg, i) in zip(stub_func.args, cfunc.type().args())]
        callee_frame = self.llvmMakeFunctionFrame(func, callee_args, stub_return_func, frame, stub_excpt_func, frame, llvm_func=stub_func, stack=frame)
        callee = self.llvmFunction(func)
        
        result = self.llvmSafeCall(callee, [callee_frame.fptr(), callee_frame.eoss(), ctx])
        result.calling_convention = llvm.core.CC_FASTCALL
        
        _makeEpilog(stub_func, dummyfunc, frame, result_id)
        
        self.popBuilder() # function body.
    
        #### Create the resume function. 
  
        resume_block = resume_func.append_basic_block("")
        self.pushBuilder(resume_block)
        
           # Restore information.
        cont = self.llvmExceptionContinuation(resume_func.args[0])
        frame = self.llvmExceptionFrame(resume_func.args[0])
        
        succ = self.llvmContinuationSucc(cont)
        succ_frame = self.llvmContinuationFrame(cont)
        
        self.llvmFrameClearException(frame)

           # Do the call.
        ctx = self._llvmGlobalExecutionContextPtr()
        call = self.llvmSafeCall(succ, [succ_frame.fptr(), succ_frame.eoss(), ctx])
        call.calling_convention = llvm.core.CC_FASTCALL
        
        _makeEpilog(resume_func, dummyfunc, frame, result_id)
        
        self.popBuilder()
        
        ### Done.
        
        return (stub_func, resume_func)

    ## Init-time constructores.
        
    def llvmNewGlobalCtor(self, callback):
        """Registers code to be executed at program startup.
        
        The code itself needs to be generated by the *callback*, which should
        use the current ~~builder. *callback* will receive a single arguments
        with the current ~~CodeGen.
        
        callback: Function - Function receiving a single parameter.
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
        callback(self)
        self.builder().ret_void()
        
        self.popBuilder()
        self._llvm.func = save_current_func
        self._llvm.builders = save_builders
        
        self._llvm.global_ctors += [function]

    def llvmRegisterGlobalCtor(self, func):
        """Registers an LLVM function to be executed at initialization time. 
        
        func: llvm.core.Value - The LLVM function.
        """
        self._llvm.global_ctors += [func]

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
        global_ctor = self.llvmTypeInternal("__hlt_global_ctor")        
        array = llvm.core.Constant.array(global_ctor, array)
        
        glob = self.llvmCurrentModule().add_global_variable(array.type, "llvm.global_ctors")
        glob.global_constant = True
        glob.initializer = array
        glob.linkage = llvm.core.LINKAGE_APPENDING
        
    ### Operands and operators.
        
    def llvmUnpack(self, t, begin, end, fmt, arg=None):
        """
        Unpacks an instance of a type from binary data. 
        
        t: ~~HiltiType - The type of the object to be unpacked from the input. 
        
        begin: llvm.core.value - The byte iterator marking the first input
        byte.
        
        end: llvm.core.value - The byte iterator marking the position one
        beyond the last consumable input byte. *end* may be None to indicate
        unpacking until the end of the bytes object is encounted.
        
        fmt: ~~Operand or string - Specifies the binary format of the input
        bytes. It can be either an ~~Operand referencing one of the
        ``Hilti::Packed`` labels, or a string specifying the name of a label
        directly.
        
        arg: ~~Operand - Additional format-specific parameter,
        required by some formats. 
        
        Returns: tuple (val, iter) - A Python tuple in which *val* is the
        unpacked value of type ``llvm.core.value`` and ``iter`` is an
        ``llvm.core.value`` corresponding to a byte iterator pointing one
        beyond the last consumed input byte (which can't be further than
        *end*). 
        """

        assert not inspect.isclass(t)
        assert isinstance(t, type.Unpackable)
        
        packed = self.currentModule().scope().lookup("Hilti::Packed")
        assert packed and isinstance(packed, id.Type)
        assert isinstance(packed.type(), type.Enum)

        if isinstance(fmt, operand.Operand):
            assert fmt.type() == packed.type()
            
        elif isinstance(fmt, str):
            label = self.currentModule().scope().lookup("Hilti::Packed::%s" % fmt)
            assert label
            fmt = operand.ID(label)
            
        else:
            print repr(fmt)
            assert False
            
        if not end:
            import hilti.instructions.bytes
            end = hilti.instructions.bytes.llvmEnd(self)

        (val, iter) = t.llvmUnpack(self, begin, end, fmt, arg)
         
        util.check_class(val, llvm.core.Value, "llvmUnpack")
        util.check_class(iter, llvm.core.Value, "llvmUnpack")
        return (val, iter)
        
    def llvmOp(self, op, coerce_to=None):
        """Converts an instruction operand into an LLVM value.
        
        op: ~~Operand - The operand to be converted.
        
        coerce_to: ~~Type - An optional refined type to convert the loaded
        operand to. 
                
        Returns: llvm.core.Value - The LLVM value of the operand that can then
        be used, e.g., in subsequent LLVM load and store instructions.
        """
        assert not coerce_to or isinstance(coerce_to, type.Type)
        
        if coerce_to:
            
            if not op.canCoerceTo(coerce_to):
                util.internal_error("llvmOp: cannot coerce %s to %s" % (op.type(), coerce_to))
                
            op = op.coerceTo(self, coerce_to)
        
        val = op.llvmLoad(self)
        val._value = op
        return val

    ### Generating debug output.
    
    _DbgCnt = 0

    def llvmDebugPrintStr(self, s, builder=None):
        if not builder:
            builder = self.builder()
        
        loc = " [%s]" % self._ins.location() if self._ins else ""
            
        CodeGen._DbgCnt += 1
        const = self.llvmNewGlobalStringConst("debug-string-%d" % CodeGen._DbgCnt, s + loc)
        func = self.llvmCFunctionInternal("__hlt_debug_print_str")
        builder.call(func, [const.bitcast(self.llvmTypeGenericPointer())])

    def llvmDebugPrintPtr(self, s, ptr):
        CodeGen._DbgCnt += 1
        const = self.llvmNewGlobalStringConst("debug-string-%d" % CodeGen._DbgCnt, s)
        ptr = self.builder().bitcast(ptr, self.llvmTypeGenericPointer())
        self.llvmCallCInternal("__hlt_debug_print_ptr", [const.bitcast(self.llvmTypeGenericPointer()), ptr])
        
    def llvmDebugPushIndent(self):
        self.llvmCallCInternal("__hlt_debug_push_indent", [])
        
    def llvmDebugPopIndent(self):
        self.llvmCallCInternal("__hlt_debug_pop_indent", [])

    def llvmDebugPrintFrame(s, frame):
        s = "   | " + s
        self.llvmDebugPrintPtr(s + "       fptr", frame.fptr())
        self.llvmDebugPrintPtr(s + "       eoss", frame.eoss())
        
        frame = self.llvmFrameNormalFrame(frame)
        
        self.llvmDebugPrintPtr(s + "parent fptr", frame.fptr())
        self.llvmDebugPrintPtr(s + "parent eoss", frame.eoss())

        

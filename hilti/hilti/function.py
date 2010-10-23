# $Id$

builtin_id = id

import block
import copy
import id
import function
import node
import operand
import scope
import type
import util
import constant

import hilti.instructions.debug
import hilti.instructions.flow

class CallingConvention:
    """The *calling convention* used by a ~~Function specifies which
    implementation calls to it use.
    """
    
    HILTI = 1
    """A ~~Function using the proprietary HILTI calling convention. This
    convetion supports all of HILTI's flow-control features but cannot
    (directly) be called from other languages such as C. This is the default
    calling convention.
    
    See :ref:`interfacing-with-C` for more information about calling
    conventions.
    """
    
    C = 2
    """A ~~Function using standard C calling convention. The
    conventions implements standard C calling semantics but does not
    support any HILTI-specific flow-control features, such as
    exceptions and continuations.
    
    See :ref:`interfacing-with-C` for more information about calling
    conventions.
    
    Todo: Implementing C functions in HILTI is not fully supported at the
    moment. For example, they can't have local variables for now because the
    code-generator would try to locate them in the HILTI stack frame, whcih
    doesn't exist. 
    """
    
    C_HILTI = 3
    """A ~~Function using C calling convention but receiving
    specially crafted parameters to support certain HILTI features.
    C_HILTI works mostly as the ~C convention yet with the following
    
    See :ref:`interfacing-with-C` for more information about calling
    conventions.
    """

@type.hilti(None, -1)                
class Function(type.HiltiType):
    """Type for functions. 
    
    args: list of either ~~IDs or of (~~ID, default) tuples - The function's
    arguments, with *default* being optional default values in the latter
    case. *default* can also be set to *None* for no default.
    
    resultt - ~~Type - The type of the function's return value (~~Void for none). 
    """
    def __init__(self, args, resultt, location=None):
        super(type.Function, self).__init__(location)
        type.Type.setTypeName("void")

        if args and isinstance(args[0], id.ID):
            self._ids = args
            self._defaults = [None] * len(args)
        else:
            self._ids = [i for (i, default) in args]
            self._defaults = [default for (i, default) in args]
            
        self._result = resultt

    def args(self):
        """Returns the functions's arguments.
        
        Returns: list of ~~ID - The function's arguments. 
        """
        return self._ids

    def argsWithDefaults(self):
        """Returns the functions's arguments with any default values.
        
        Returns: list of (~~ID, default) - The function's arguments with their
        default values. If there's no default for an arguments, *default*
        will be None.
        """
        return zip(self._ids, self._defaults)
    
    def getArg(self, name):
        """Returns the named function argument.
        
        Returns: ~~ID - The function's argument with the given name, or None
        if there is no such arguments. 
        """
        for id in self._ids:
            if id.name() == name:
                return id
            
        return None
    
    def resultType(self):
        """Returns the functions's result type.
        
        Returns: ~~Type - The function's result. 
        """
        return self._result

    def setResultType(self, rt):
        """Sets the function's result type.
        
        rt: ~~Type - The new result type.
        """
        self._result = rt
    
    ### Overridden from Type.

    def name(self):
        args = ", ".join([str(i.type()) for (i, default) in zip(self._ids, self._defaults)])
        return "function (%s) -> %s" % (args, self._result)
        
    def _resolve(self, resolver):
        for i in self._ids:
            i.resolve(resolver)
            
        for d in self._defaults:
            if d:
                d.resolve(resolver)

        if self._result:
            self._result = self._result.resolve(resolver)
                
        return self
            
    def _validate(self, vld):
        for i in self._ids:
            i.validate(vld)
            
        if not isinstance(self._result, type.ValueType):
            vld.error(self, "return type must be a value type but is %s" % self._result)
            return 
            
        for d in self._defaults:
            if d:
                d.validate(vld)

    ### Overridden from HiltiType.
    
    def canCoerceTo(self, dsttype):
        return self == dsttype or dsttype.__class__.__name__ == "CAddr"
    
    def llvmCoerceTo(self, cg, value, dsttype):
        assert self.canCoerceTo(dsttype)
        return value

    ### Overidden from node.Node.

    def output(self, printer):
        printer.printType(self.resultType())
        printer.output(" %s(" % self._name)
        
        first = True
        for (arg, default) in self.argsWithDefaults():
            
            if not first:
                printer.output(", ")
            
            arg.output(printer)
            
            if default:
                printer.output(" = ")
                default.output(printer)
            
            first = False
                
        printer.output(")", nl=False)
    
class Function(node.Node):
    """A HILTI function declared or defined inside a module.
    
    name: string - Name of the function. It must be *non-qualified*,
    i.e., not be prefixed with any module name.
    
    ty: ~~Function - The type of the function.
    
    ident: ~~ID - The ID to which the function is bound. *id* can be initially
    None, but must be set via ~~setID before any other methods are used.
    
    parent: ~~Scope - The parent scope to lookup IDs outside of the function's
    scope. Can be None if *scope* is given. 

    location: ~~Location - A location to be associated with the function. 
    """
    
    def __init__(self, ty, ident, parent, cc=CallingConvention.HILTI, location=None):
        assert isinstance(ty, type.Function)
        
        super(Function, self).__init__(location)
        
        self._bodies = []
        self._cc = cc
        self._type = ty
        self._id = ident
        self._handlers_cnt = 1
        self._handlers = []
        self._output_name = None
        self._hook = False

        self._scope = scope.Scope(parent)
        for i in ty.args():
            self._scope.add(copy.copy(i))
        
    def name(self):
        """Returns the function's name.
        
        Returns: string - The name of the function.
        """
        return self._id.name()

    def scope(self):
        """Returns the module's scope. 
        
        Returns: ~~Scope - The scope. 
        """
        return self._scope

    def locals(self):
        """Returns a list of a local IDs in the function's scope.  The list
        will start with all local parameters in the order as defined in the
        function's declaration; followed by a list of all local variables in
        undefined (yet stable) order.
        
        Returns: list of id.Local - The locals.
        """
        params = self._type.args()
        locals = [i for i in self.scope().IDs() if isinstance(i, id.Local) and not isinstance(i.type(), type.Label)]
        return params + sorted(locals, key=lambda i: i.name())
    
    def type(self):
        """Returns the type of the function. 
        
        Returns: :class:`~hilti.type.Function` - The type of the
        function.
        """
        return self._type

    def id(self): 
        """Returns the function's ID. 
        
        Returns: ~~ID - The ID to which the function is bound. 
        """
        return self._id
    
    def setID(self, i): 
        """Sets the ID to which the function is bound.
        
        id: ~~ID - The ID.
        """
        self._id = i
        
    def setHookFunction(self):
        """Marks the function as being associated with a hook as one of its
        functions."""
        self._hook = True
        
    def hookFunction(self):
        """Return whether the function is associated with a hook as one its
        hook functions.
        
        Return: bool - True if a hook function."""
        return self._hook
        
    def setOutputName(self, name):
        """Sets an alternate name for the function to be used when printing it
        as HILTI source code.
        
        name: string - The alternative name.
        """
        self._output_name = name
        
    def callingConvention(self):
        """Returns the calling convention used by the function.
        
        Returns: ~~CallingConvention - The calling convention used by the
        function."""
        return self._cc

    def setCallingConvention(self, cc):
        """Sets the function's calling convention.
        
        cc: ~~CallingConvention - The calling convention to set.
        """
        self._cc = cc

    def addBlock(self, b):
        """Adds a block to the function's implementation. Appends the ~~Block
        to the function's current list of blocks, and makes the new Block the
        successor of the former tail Block. The new blocks own successor field
        is cleared. The block's name is added to the function's scope.
        
        b: ~~Block - The block to add.
        """
        
        if b.name() and self.scope().lookup(b.name()):
            util.internal_error("block name %s already defined" % b.name())
        
        if self._bodies:
            self._bodies[-1].setNext(b.name())
            
        self._bodies += [b] 
        b.setNext(None)
        
        self._scope.add(id.Local(b.name(), type.Label()))
    
    def blocks(self):
        """Returns all blocks making up the function's implementation.
        
        Returns: list of ~~Block - The function's current list of blocks;
        may be empty.
        """
        return self._bodies

    def clearBlocks(self):
        """Clears the function's implementation. All blocks that have been
        added to function previously, are removed. Their IDs are also removed
        from the function's scope.
        """
        for b in self._bodies:
            self._scope.remove(id.Local(b.name(), type.Label()))
        
        self._bodies = []

    def lookupBlock(self, name):
        """Lookups the block with the given name. 
        
        name: string - The name to lookup. 
        
        Returns: ~~Block - The block with the *id's* name, or None if no such
        block exists.
        """
        for b in self._bodies:
            if b.name() == name:
                return b
        return None

    def pushExceptionHandler(self, excpt, handler):
        """Pushed a new exception handler on the the handler stack. If 
        subsequent code raised the given exception, or any derived from it,
        control is transfered to a handler block. Note the if multiple
        handlers match a raised exception, the one pushed most recently wins
        and will be executed.
        
        Can only be used during code generation.
        
        cg: ~~CodeGen - The current code generator.
        
        excpt: ~~Exception - The exception type to catch.
        
        handler: ~~Block - The block to execute when an exception of type
        ~~excpt is raised.
        """
        util.check_class(excpt, type.Exception, "pushExceptionHandler")
        util.check_class(handler, block.Block, "pushExceptionHandler")
        
        # We assign unique IDs to each handler, which we can later use to
        # generate unique labels.
        self._handlers += [(excpt, handler, self._handlers_cnt)]
        self._handlers_cnt += 1

    def popExceptionHandler(self):
        """Pops the most recently push exception handler from the handler stack.
        
        Can only be used during code generation.
        """
        # We pop all with the same number, as they all have been added with the
        # last push.
        assert self._handlers
        self._handlers = self._handlers[0:-1]

    def llvmHandleException(self, cg):
        """Builds code for finding the right handler for the current exception
        as set in the current function's frame. The code first checks whether
        we have any matching handler defined via ~~pushExceptionHander(); if
        so, it transferts control to there.  If not, it escalates the
        exception to its caller. 
        
        The method uses the code generation's current builder. Any code build
        subsequently with the same builder will never be reached. 
        
        The method must only be called if *self* is the code generators
        current function. 
        
        cg: ~~CodeGen - The current code generator.
        """
        assert builtin_id(self) == builtin_id(cg.currentFunction())
        
        if self._handlers:
            cg.llvmTailCall(self.llvmExceptionHandler(cg))
        
        else:
            # No handlers, escalate directly to caller. 
            excpt = cg.llvmFrameException()
            cg.llvmEscalateException(excpt)
        
        
    def llvmExceptionHandler(self, cg):
        """Returns an handler to handle exception raised at the current
        location during code generation. If handlers have been installed
        via ~~pushExceptionHandler, corresponding code is built and a
        function containing it is returned. If no handlers have been
        installed, we return our parent function's handler. 
        
        The returned function is suitable to store in a function's frame
        via ~~llvmMakeFunctionFrame's ``contExceptFunc`` argument. 
        
        The method must only be called if *self* is the code generators
        current function. 
    
        cg: ~~CodeGen - The current code generator.
        
        Returns: llvm.core.Value - The handler function. 
        """
        def _makeOne(func, idx):
            # Generates code for checking whether self._handlers[idx] or any
            # of it predecessors applies. Returns the *block* to which we
            # can jump to execute the code.
            check = cg.llvmNewBlock("match")
            cg.pushBuilder(check) 
            
            excpt = cg.llvmFrameException()
            
            if idx < 0:
                # No further handler to check, send to caller. 
                cg.llvmEscalateException(excpt)
                
            else:
                (etype, handler, num) = self._handlers[idx]
                
                # Build code for calling handler in case of a match.
                found = cg.llvmNewBlock("found")
                cg.pushBuilder(found) 
                cg.llvmFrameClearException()
                cg.llvmTailCall(handler)
                cg.popBuilder()
                
                # See if the given handler matches the exception's type.
                cmp = cg.llvmCallCInternal("__hlt_exception_match_type", [excpt, cg.llvmObjectForExceptionType(etype)])
                cg.builder().cbranch(cmp, found, _makeOne(func, idx-1))
                
            cg.popBuilder()
            return check

        if not self._handlers:
            # No handlers defined, pass on to parent.
            return cg.llvmFrameExcptSucc()
        
        name = "%s_exception_%s" % (cg.nameFunction(self), self._handlers[-1][2])
        
        def _makeFunction():
            # Build the whole function.
            func = cg.llvmNewHILTIFunction(self, name=name)
            
            # TODO: This is clearly a hack. Need to build a better interface
            # for building a function while another is in progress.
            old_func = cg._llvm.func
            old_frameptr = cg._llvm.frameptr
            old_eoss = cg._llvm.eoss
            old_ctxptr = cg._llvm.ctxptr
            
            cg._llvm.func = func
            cg._llvm.frameptr = func.args[0]
            cg._llvm.eoss = func.args[1]
            cg._llvm.ctxptr = func.args[2]
            
            _makeOne(func, len(self._handlers) - 1)
            
            cg._llvm.func = old_func
            cg._llvm.frameptr = old_frameptr
            cg._llvm.eoss = old_eoss
            cg._llvm.ctxptr = old_ctxptr
            
            return func

        return cg.cache(name, _makeFunction)
            

    def __str__(self):
        return "%s" % self._id.name()

    ### Overridden from Node.
    
    def _resolve(self, resolver):
        resolver.startFunction(self)
        
        if self._type:
            self._type = self._type.resolve(resolver)
            
        self._scope.resolve(resolver)
        
        for b in self._bodies:
            b.resolve(resolver)

        resolver.endFunction()
            
    def _validate(self, vld):
        vld.startFunction(self)
        self._type.validate(vld)
        self._scope.validate(vld)
        
        for b in self._bodies:
            b.validate(vld)
    
        if self.callingConvention() == CallingConvention.HILTI:
            for i in self.type().args():
                t = i.type()
                if isinstance(t, type.ValueType) and not t.instantiable():
                    vld.error(self, "HILTI functions cannot have parameter of type %s" % t)
                    return
                
            t = self.type().resultType()
            if isinstance(t, type.ValueType) and not t.instantiable() and not isinstance(t, type.Void):
                vld.error(self, "HILTI functions cannot have result of type %s" % t)
                return
            
        vld.endFunction()
            
    def output(self, printer):
        printer.printComment(self)
        
        ftype = self._type
        
        if not self.blocks():
            printer.output("declare ")
        
        cc = ""
        
        if self.callingConvention() == CallingConvention.C:
            cc += "\"C\" "
                
        elif self.callingConvention() == CallingConvention.C_HILTI:
            cc += "\"C-HILTI\" "

        printer.output(cc)
        
        ftype._name = self._id.name() if not self._output_name else self._output_name
        ftype.output(printer)

        self._outputAttrs(printer)
        
        if not self.blocks():
            printer.output("", nl=True)
            return
        
        printer.output(" {", nl=True)
        printer.push()
        
        locals = False
        for i in sorted(self.scope().IDs(), key=lambda i: i.name()):
            if isinstance(i, id.Local) and not isinstance(i.type(), type.Label):
                printer.output("local ")
                i.output(printer)
                printer.output()
                locals = True
                
        if locals:
            printer.output()
                
        first = True
        for block in self.blocks():
            if not first:
                printer.output()
                
            block.output(printer)
            first = False
                
        printer.output("}", nl=True)        
        printer.output("", nl=True)        
        printer.pop()

    def _outputAttrs(self, printer):
        # Overwritten by derived classes to print out attributes.
        pass
        
    def _canonify(self, canonifier):
        """
        * Blocks that don't have a successor are linked to the block following them
          within their function's list of ~~Block objects, if any.
          
        * All blocks will end with a |terminator|. If none is already in
          place, we either add a ~~Jump pointing the succeeding block (as
          indicated by :meth:`~hilti.block.Block.next`), or a ~~ReturnVoid if
          there is no successor. In the latter case, the function must be of
          return type ~~Void. 
          
        * Each so far name-less block gets a name which is unique at least within the
          function the block is part of.
          
        * Empty blocks are removed if ~~mayRemove indicates that it is permissible to
          do so.  
        """
        
        canonifier.startFunction(self)

        # self._scope.canonify(canonifier)
        self._id.canonify(canonifier)

        if canonifier.debugMode() > 1:
            args = [operand.ID(i) for i in canonifier.currentFunction().type().args()]
            fmt = ["%s"] * len(args)
            dbg = hilti.instructions.debug.message("hilti-flow", "entering %s(%s)" % (canonifier.currentFunctionName(), ", ".join(fmt)), args)
            
            if self.blocks():
                self.blocks()[0].addInstructionAtFront(dbg)
            else:
                # Function without an implementation.
                pass
                
        # "init" functions get C linkage.
        if self.id().linkage() == id.Linkage.INIT:
            self.setCallingConvention(function.CallingConvention.C)
            
        # Chain blocks together where not done yet.
        prev = None
        for b in self.blocks():
            if prev and not prev.next():
                prev.setNext(b.name())
                
        # Canonify blocks. This will add blocks to the transformed chain.
        for b in self.blocks():
            b.canonify(canonifier)
                
        # Copy the new blocks over.
        self.clearBlocks()
        for b in canonifier.transformedBlocks():
            if b.instructions() or not b.mayRemove():
                _unifyBlock(canonifier, b)
                self.addBlock(b)

        # Chain blocks  where not done yet; again because canonicalization may
        # have added new blocks. 
        prev = None
        for b in self.blocks():
            if prev and not prev.next():
                prev.setNext(b.name())
                
        canonifier.endFunction()
        
    def _codegen(self, cg):
        cg.startFunction(self)
        
        if self.id().linkage() == id.Linkage.EXPORTED and self.callingConvention() == CallingConvention.HILTI:
            cg.llvmGenerateCStubs(self)
        
        if self.id().linkage() == id.Linkage.INIT:
            def _makeCall(cg):
                # We register the function to be called from hilti_init() later,
                # rather than calling it directly here. That ensures that all the
                # HILTI internal init stuff gets executed first.
                llvm_func = cg.llvmFunction(self)
                llvm_func = cg.builder().bitcast(llvm_func, cg.llvmTypeGenericPointer())
                llvm_func = operand.LLVM(llvm_func, self.type())
                cg.llvmCallC("hlt::register_init_function", [llvm_func], abort_on_except=True)
        
            cg.llvmNewGlobalCtor(_makeCall)

        if self._bodies:
            cg.trace("### bgn %s" % self)
            for b in self._bodies:
                b.codegen(cg)
            cg.trace("### end %s" % self)
        
        cg.endFunction()

def _unifyBlock(canonifier, block):
    # If the last instruction is not a terminator, add it. 
    instr = block.instructions()
    loc = block.function().location() if block.function() else None
    
    add_terminator = False

    if not len(instr):
        add_terminator = True
        
    if len(instr) and not instr[-1].signature().terminator():
        add_terminator = True
        
    if add_terminator:
        import hilti.instructions.hook
        import hilti.instructions.flow
        if block.next():
            newins = hilti.instructions.flow.Jump(operand.ID(id.Local(block.next(), type.Label(), location=loc)), location=loc)
        else:
            
            if canonifier.debugMode() > 1:
                dbg = hilti.instructions.debug.message("hilti-flow", "leaving %s" % canonifier.currentFunctionName())
                block.addInstruction(dbg)

            if isinstance(canonifier.currentFunction(), hilti.instructions.hook.HookFunction):
                newins = hilti.instructions.flow.ReturnResult(operand.Constant(constant.Constant(0, type.Bool())), location=loc)
                newins.setInternal()
            else:
                newins = hilti.instructions.flow.ReturnVoid(None, location=loc)

        block.addInstruction(newins)

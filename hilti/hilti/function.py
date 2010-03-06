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

    ### Overridden from Type.

    def name(self):
        args = ", ".join([str(i.type()) for (i, default) in zip(self._ids, self._defaults)])
        return "function (%s) -> %s" % (args, self._result)
        
    def resolve(self, resolver):
        for i in self._ids:
            i.resolve(resolver)
            
        for d in self._defaults:
            if d:
                d.resolve(resolver)

        if self._result:
            self._result = self._result.resolve(resolver)
                
        return self
            
    def validate(self, vld):
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

        self._scope = scope.Scope(parent)
        for i in ty.args():
            self._scope.add(copy.deepcopy(i))
        
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
    
    def __str__(self):
        return "%s" % self._id.name()

    ### Overridden from Node.
    
    def resolve(self, resolver):
        if self._type:
            self._type = self._type.resolve(resolver)
            
        self._scope.resolve(resolver)
        
        for b in self._bodies:
            b.resolve(resolver)

    def validate(self, vld):
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
        printer.printType(ftype.resultType())
        printer.output(" %s(" % self._id.name())
        
        first = True
        for (arg, default) in ftype.argsWithDefaults():
            if not first:
                printer.output(", ")
            
            arg.output(printer)
            
            if default:
                printer.output(" = ")
                default.output(printer)
            
            first = False
                
        if not self.blocks():
            printer.output(")", nl=True)
            return
        
        printer.output(") {", nl=True)
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
        printer.pop()
        
    def canonify(self, canonifier):
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

        if canonifier.debugMode():
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
                
        canonifier.endFunction()
        
    def codegen(self, cg):
        cg.startFunction(self)
        
        if self.id().linkage() == id.Linkage.EXPORTED and self.callingConvention() == CallingConvention.HILTI:
            cg.llvmCStubs(self)
        
        if self.id().linkage() == id.Linkage.INIT:
            def _makeCall():
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
        if block.next():
            newins = hilti.instructions.flow.Jump(operand.ID(id.Local(block.next(), type.Label(), location=loc)), location=loc)
        else:
            if canonifier.debugMode():
                dbg = hilti.instructions.debug.message("hilti-flow", "leaving %s" % canonifier.currentFunctionName())
                block.addInstruction(dbg)

            newins = hilti.instructions.flow.ReturnVoid(None, location=loc)

        block.addInstruction(newins)

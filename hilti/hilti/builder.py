# $Id$

builtin_id = id

import ast
import block
import constant
import function
import id
import instruction
import module
import objcache
import type
import util
import operand

import hilti

class OperandBuilder(objcache.Cache):
    """A class for creating instruction operands.
    """
    def __init__(self):
        super(OperandBuilder, self).__init__()
    
    def constOp(self, value, ty=None):
        """Return a constant operand.
        
        value: any - The value of the constant. 
        ty: HiltiType - The type of the constant. For convenience, the type
        can be skipped for integers and strings and bools.
        
        Returns: ~~Constant - The constant operand.
        """
        
        if isinstance(value, bool) and not ty:
            ty = type.Bool()
            
        elif isinstance(value, int) and not ty:
            ty = type.Integer(64)
            
        elif isinstance(value, str) and not ty:
            ty = type.String()
            
        assert ty
        
        return operand.Constant(constant.Constant(value, ty))
    
    def nullOp(self):
        """Returns a constant operand representing the Null value.
        
        Returns: ~~Constant - The Null value.
        """
        return operand.Constant(constant.Constant(None, type.Reference(type.Wildcard)))
    
    def typeOp(self, type):
        """Return a type operand.
        
        type: HiltiType - The type reference by the operand.
        
        Returns: ~~Type - The type operand.
        """
        return operand.Type(type)

    def idOp(self, i):
        """Return an ID operand.
        
        i: ID - The ID to build an operand for.
        
        Returns: ID - The ID operand.
        """
        assert isinstance(i, id.ID)
        return operand.ID(i)
    
    def tupleOp(self, elems):
        """Return an tuple operand.
        
        elems: list of ~~Operand - The elements of the tuple.
        
        Returns: operand.Constant - The tuple operand made of the given elements. 
        """
        const = constant.Constant(elems, type.Tuple([e.type() for e in elems]))
        return operand.Constant(const)

class ModuleBuilder(OperandBuilder):
    """A class for adding entities to a module.
    
    module: Module - The model going to be extended.
    """
    def __init__(self, module):
        super(ModuleBuilder, self).__init__()
        self._module = module

    def module(self):
        """Returns the module associated with this builder.
        
        Returns: Module - The module.
        """
        return self._module

    def finish(self, validate=True): 
        """Finishes the building process. This must be called after all
        elements have been added to the module to guarantee a correct module.
        It also checks the semantic correctness of the module, and if it find
        any problems it reports them to stderr.
        
        validate: bool - If false, the HILTI validation is not
        performed.
        
        Returns: integer - The number of errors found. 
        """

        resolver_errors = hilti.resolveModule(self._module)
        
        if resolver_errors:
            return resolver_errors

        if validate:
            errors = hilti.validateModule(self._module)
            if errors:
                return errors
        
        return 0
    
    def importModule(self, name):
        """Imports another module. This has the same effect as a HILTI
        ``import`` statement.
        
        name: string - The name of the module.
        
        Returns: bool - True iff the module has been imported successfully.
        
        Todo: This function has not been implemented yet. 
        """
        assert False and "import not implemented yet"
    
    def idOp(self, i):
        """Returns an ID operand. Different from ~~OperandBuilder.idOp, this
        method can lookup the ID by name in  the module's scope.
        
        i: ID or string - The ID, or the name of the ID which is then looked
        up in the module's scope. In the latter case, the ID *must* exist. 
        
        Returns: ID - The ID operand.
        """
        if isinstance(i, str):
            li = self._module.scope().lookup(i)
            if not li:
                raise ValueError("ID %s does not exist in function's scope" % i)

            return operand.ID(li)
            
        else:
            return OperandBuilder.idOp(self, i)

    def addConstant(self, name, type, value):
        """Adds a new global constant to the module scope's.
        
        name: string - The name of the global ID. 
        type: HiltiType - The type of the global ID.
        value: any - The value the constant; it's type must correspond to whatever *type* demands. 
        
        Returns: ~~ID - An ID operand referencing the constant. 
        """
        i = id.Constant(name, constant.Constant(value, type))
        self._module.scope().add(i)
        return operand.ID(i)
    
    def addGlobal(self, name, type, value):
        """Adds a new non-constant global to the module scope's.
        
        name: string - The name of the global ID. 
        type: HiltiType - The type of the global ID.
        value: ~~Operand - The value the global.
        
        Returns: ~~ID - An ID operand referencing the constant. 
        """
        i = id.Global(name, type, value)
        self._module.scope().add(i)
        return operand.ID(i)
        
    def addTypeDecl(self, name, ty):
        """Adds a new global type declaration to the module scope's.
        
        name: string - The name of the global ID for the type declaration. 
        ty: HiltiType - The type being declared. 
        
        Returns: ~~Type - A type operand referencing the declaration. 
        """
        i = id.Type(name, ty)
        self._module.scope().add(i)        
        return self.typeOp(ty)
    
class FunctionBuilder(OperandBuilder):
    """A class for building functions. 
    
    mbuilder: ModuleBuilder - The builder for the module the function will become part of.
    name: string - The name of the function.
    ftype: type.Function - The type of the function.
    cc: function.CallingConvention - The function's calling convention.
    location: Location - The location to be associated with the function.
    """
    def __init__(self, mbuilder, name, ftype, cc=function.CallingConvention.HILTI, location=None):
        super(FunctionBuilder, self).__init__()
        
        self._func = function.Function(ftype, None, mbuilder.module().scope(), cc, location)
        i = id.Function(name, ftype, self._func)
        self._func.setID(i)
        self._mbuilder = mbuilder
        self._tmps = {}
        mbuilder.module().scope().add(i)
        
    def function(self):
        """Returns the HILTI function being build.
        
        Returns: Function - The function.
        """
        return self._func

    def moduleBuilder(self):
        """Returns the builder for the module the function being built is part
        of.
        
        Returns: ~~ModuleBuilder - The module builder.
        """
        return self._mbuilder

    def cacheBuilder(self, name, generate):
        """XXX"""
        def _makeBlock():
            def _do():
                builder = BlockBuilder(name, self)
                generate(builder)
                return builder
            
            return _do
            
        return self.cache("_block-%s" % name, _makeBlock())

    def newBuilder(self, name):
        return BlockBuilder(name, self)
    
    def idOp(self, i):
        """Returns an ID operand. Different from ~~OperandBuilder.idOp, this
        method can lookup the ID by name in function's scope (including the
        global module scope). 
        
        i: ID or string - The ID, or the name of the ID which is then looked
        up in the module's scope. In the latter case, the ID *must* exist. 
        
        Returns: ID - The ID operand.
        """
        if isinstance(i, str):
            li = self._func.scope().lookup(i)
            if li:
                return operand.ID(li)            
                
            return self._mbuilder.idOp(i)
        
        else:
            return OperandBuilder.idOp(self, i)

    def typeByID(self, name):
        """Returns the type referenced by the an ID name. If the referenced
        type is a ~~HeapType, returns a ~~Reference to it. 
        
        name: string - The name of the ID; it must be an ~~id.Type. 
        
        Returns: hilti.type - The type referenced by the ID.
        """
        li = self.moduleBuilder().module().scope().lookup(name)
        if not li:
            raise ValueError("ID %s does not exist in function's scope" % i)

        if not isinstance(li, id.Type):
            raise ValueError("ID %s is not a type declaration" % i)
        
        ty = li.type()
        
        if isinstance(ty, type.HeapType):
            return type.Reference(ty)
        else:
            return ty
        
    def addLocal(self, name, ty, value = None, force=True, reuse=False):
        """Adds a new local variable to the function. Normally, it's ok if the
        local already exists, the name is then adapted to be unique. However,
        if *force* is set to false, it is an error if a variable under that
        name already exists.  If *force* is False and *reuse* is set, the
        exisiting local is returned (which must have type *ty*). 
        
        name: string - The name of the variable.
        
        ty: ~~ValueType or ~~ID - The type of the variable.
        If an ~~ID, it must be a corresponding type
        declaration.
        
        value: ~~Constant - Optional initialization of the local.  Note that
        if the local already exists, it's current value will be overridden.
        
        force: ~~bool - Adapt the name to be unique if necssary. 
        
        Returns: ~~ID - An ID referencing the new local. 
        
        Note: There's ~~addTmp if you need only a temporary that doesn't need
        to remain valid very long
        """
        
        assert not (reuse and force)
        
        old = self._func.scope().lookup(name)
        
        if old:
            if reuse:
                return old
            
            if not force:
                util.internal_error("ID %s already exists" % name)
            else:
                # Generate a unique name.
                name = self._idName(name)
        
        if isinstance(ty, operand.ID):
            assert isinstance(ty.id(), id.Type)
            ty = ty.id()
            
        i = id.Local(name, ty)
        self._func.scope().add(i)
        op = self.idOp(i)    
        
        if value:
            self.assign(op, value)
            
        return op
        
    def addTmp(self, name, ty):
        """Creates a temporary local variable. The variable can then be used
        for storing intermediary results until this method is called the next
        time with the same name and type; in that case, the existing local
        will be returned. If the method is later called with the same name but
        a different type, the name will be adapted to be unique (and
        subsequent calls with the same name/type combination will return the
        local with the adapted name). 
        
        name: string - The name of the temporary.
        
        ty: ~~Type - The type of the temporary.
        
        Returns: ~~hilti.operand.ID - An ID
        referencing the temporary.
        
        Note: There's ~~addLocal if you need a variable that will remain valid
        longer.
        """
        
        idx = "%s|%s" % (name, ty)

        try:
            return self._tmps[idx]
        except KeyError:
            pass
        
        tmp = self.addLocal(name, ty, force=True)
        
        self._tmps[idx] = tmp
        return tmp
        
    def _idName(self, name=""):
        if name == None:
            return None

        name = name.replace("-", "_")
        
        i = 1
        nname = name
        
        while self._func.scope().lookup(nname):
            i += 1
            nname = "%s%d" % (name, i)

        return nname

class BlockBuilder(OperandBuilder): 
    """A class for building blocks of instruction in memory. A ~~BlockBuilder
    supports all HILTI instructions as method calls. In general, an
    instruction of the form ``<target> = <part1>.<part2> <op1> <op2>``
    translated into a builder method ``<part1>_<part2>(<target>, <op1>,
    <op2>)`` where ``<target>`` and ``<op1>/<op2>`` are instances of
    ~~operand.Operand. If an instruction has target, the method returns
    the target operand. (Note that these methods are not documented in the
    ~~Builder's method reference.)

    name: string or None - A label for the new block. If None, the block will
    not have a label associated with it. Note the that generated label might
    be different if a block of that name already exists; the new block will
    always have a unique name. 
    
    fbuilder: ~~FunctionBuilder  - The builder for the function the block
    being built is part of. 
    """
    def __init__(self, name, fbuilder):
        super(BlockBuilder, self).__init__()
        self._fbuilder = fbuilder
        name =  fbuilder._idName("@%s" % name) if name else ""
        self._block = block.Block(self._fbuilder.function(), name=name)
        self._fbuilder.function().addBlock(self._block)
        self._next_comment = []
        
    def block(self):
        """Returns the block this builder is adding instructions to.
        
        Returns: Block - The block.
        """
        return self._block

    def functionBuilder(self):
        """Returns the builder for the function the block being built is part
        of.
        
        Returns: ~~FunctionBuilder - The function builder.
        """
        return self._fbuilder

    def addLocal(self, name, ty, value=None, force=False, reuse=False):
        """Adds a new local variable to the function. This is just a
        convinience function that forwards to
        ~~FunctionBuilder.addLocal. See there for more information.
        """
        return self._fbuilder.addLocal(name, ty, value, force, reuse)

    def addTmp(self, name, ty):
        """Creates a temporary local variable. This is just a
        convinience function that forwards to
        ~~FunctionBuilder.addLocal. See there for more information.
        """
        return self._fbuilder.addTmp(name, ty)
    
    def idOp(self, i):
        """Returns an ID operand. Different from ~~OperandBuilder.idOp, this
        method can lookup the ID by name in the block's scope (including the
        function's scope and the global module scope). 
        
        i: ID or string - The ID, or the name of the ID which is then looked
        up in the module's scope. In the latter case, the ID *must* exist. 
        
        Returns: ID - The ID operand.
        """
        return self._fbuilder.idOp(i)
    
    def setComment(self, comment):
        """Associates a comment with block being built.
        
        commment: string - The comment
        """
        self._block.setComment(comment)

    def setNextComment(self, comment):
        """Associates a comment with the next instruction that will be added
        to the block.
        
        commment: string - The comment.
        """
        self._next_comment += [comment]
        
    def labelOp(self):
        """Returns an ID operand referencing the block being built. The block
        must have a name associated with. 
        
        Returns: ID - The ID operand refering to the block.
        """
        return self._fbuilder.idOp(self._block.name())
        
    def _label(self, tag1, tag2):
        
        if tag1:
            tag1 = "%s_" % tag1
        else:
            tag1 = ""

        return tag1 + tag2
    
    def makeIfElse(self, cond, yes=None, no=None, cont=None, tag=None):
        """Builds an If-Else construct based on a given condition. The
        function creates two ~~BlockBuilder for the two branches, which can
        then be filled with instructions. The function also creates one
        continuation block, and the two branches are setup to continue
        execution there (unless they leave the current function otherwise).
        
        All three builders can optionally be substitued with one passed in.
        Note that if *yes* or *no* are passed in, their successor block is
        *not* set to the continuation builder (i.e., they are left untouched).
        
        cond: ~~Operand - The condition operand; must be of type bool.
        yes: ~~BlockBuilder - A builder to take the role of the true branch.
        no: ~~BlockBuilder - A builder to take the role of the false branch.
        cont: ~~BlockBuilder - A builder to take the role of continuation
        branch.
        tag: string - If given, will used to generate labels for the created
        blocks.
        
        Returns (~~BlockBuilder, ~~BlockBuilder, ~~BlockBuilder) - The first
        builder is for the *true* branch, the second for the *false* branch,
        and the third for continuing execution afterwards. If any of these is
        passed in, they will be returned here; otherwise, new builders will be
        returned.
        """
        assert cond.type() == type.Bool

        yes_builder = BlockBuilder(self._label(tag, "true"), self._fbuilder) if not yes else yes
        no_builder = BlockBuilder(self._label(tag, "false"), self._fbuilder) if not no else no
        cont_builder = BlockBuilder(self._label(tag, "cont"), self._fbuilder) if not cont else cont

        if not yes:
            yes_builder.block().setNext(cont_builder.block().name())
            
        if not no:
            no_builder.block().setNext(cont_builder.block().name())
            
        self.if_else(cond, yes_builder.labelOp(), no_builder.labelOp())
        
        return (yes_builder, no_builder, cont_builder)

    def _makeIf(self, cond, invert, branch, cont, tag):
        assert cond.type() == type.Bool

        name = "false" if invert else "true"
       
        branch_builder = BlockBuilder(self._label(tag, name), self._fbuilder) if not branch else branch
        cont_builder = BlockBuilder(self._label(tag, "cont"), self._fbuilder) if not cont else cont

        if not branch:
            branch_builder.block().setNext(cont_builder._block.name())

        op1 = branch_builder.labelOp()
        op2 = cont_builder.labelOp()
        
        if invert:
            self.if_else(cond, op2, op1)
        else:
            self.if_else(cond, op1, op2)
        
        return (branch_builder, cont_builder)
    
    def makeIf(self, cond, yes=None, cont=None, tag=None):
        """Builds an If construct based on a given condition.
      
        The function creates one ~~BlockBuilder for the true branch, which can
        then be filled with instructions. The function also creates one
        continuation block, and the branch is setup to continue execution
        there (unless it leaves the current function otherwise).
        
        Both builders can optionally be substitued with one passed in. Note
        that if *yes* is passed in, its successor block is *not* set to the
        continuation builder (i.e., it is left untouched).
        
        cond: ~~Operand - The condition operand; must be of type bool.
        yes: ~~BlockBuilder - A builder to take the role of the true branch.
        cont: ~~BlockBuilder - A builder to take the role of continuation
        branch.
        tag: string - If given, will br used to generate labels for the created
        blocks.
        
        Returns (~~BlockBuilder, ~~BlockBuilder) - The first builder is for
        the *true* branch and the second for continuing execution afterwards.
        If any of these is passed in, they will be returned here; otherwise,
        new builders will be returned.
        """
        return self._makeIf(cond, False, yes, cont, tag)
    
    def makeIfNot(self, cond, no=None, cont=None, tag=None):
        """Builds an If construct based on the inverse of a given condition.
      
        The function creates one ~~BlockBuilder for the false branch, which
        can then be filled with instructions. The function also creates one
        continuation block, and the branch is setup to continue execution
        there (unless it leaves the current function otherwise).
        
        Both builders can optionally be substitued with one passed in. Note
        that if *yes* is passed in, its successor block is *not* set to the
        continuation builder (i.e., it is left untouched).
        
        cond: ~~Operand - The condition operand; must be of type bool.
        no: ~~BlockBuilder - A builder to take the role of the false branch.
        cont: ~~BlockBuilder - A builder to take the role of continuation
        branch.
        tag: string - If given, will be used to generate labels for the created
        blocks.
        
        Returns (~~BlockBuilder, ~~BlockBuilder) - The first builder is for
        the *no* branch and the second for continuing execution afterwards.
        If any of these is passed in, they will be returned here; otherwise,
        new builders will be returned.
        """
        return self._makeIf(cond, True, no, cont, tag)

    def makeSwitch(self, cond, values, default=None, branches=None, cont=None, tag=None):
        """Builds a Switch construct based on a given condition. The function
        creates one ~~BlockBuilder for each alternative branch as well as one
        for the default branch, all of which can then be filled with
        instructions. All branches are setup to continue execution with *cont*
        (unless they leave the current function otherwise)

        All default and continuation builders can optionally be substitued
        with ones passed in. Note that if *default* is passed in, its
        successor block is *not* set to the continuation builder (i.e., it is
        left untouched).

        cond: ~~Operand - The condition operand.
        values: list of ~~Operand - One operand for each possible alternative,
        representing the value to be match against *cond*. The type of all the
        operands must match that of *cond*.
        default: ~~BlockBuilder - A builder to take the role of the default branch.
        branches: list of ~~BlockBuilder - A list of builders to take the role
        of the case branches; must have same length as *values*.
        cont: ~~BlockBuilder - A builder to take the role of continuation
        branch.
        tag: string - If given, will used to generate labels for the created
        blocks.
        
        Returns (~~BlockBuilder, list of ~~BlockBuilder, ~~BlockBuilder) - The
        first builder is for the default branch; the list of builders has one
        builder per alternative with their order corresponding to that of
        *values*; and the final builder is for continuing execution afterwards
        independent of the condition.  If any of these is passed in, they will
        be returned here; otherwise, new builders will be returned.
        """
        
        for v in values:
            assert v.canCoerceTo(cond.type())

        alts = []
        
        if not branches:
            branches = []
            for i in range(len(values)):
                b = BlockBuilder(self._label(tag, "case_%d" % i), self._fbuilder)
                b.setComment("Case: %s" % values[i])
                branches += [b]
                
        assert len(values) == len(branches)
        alts = [self.tupleOp((v, b.labelOp())) for (v, b) in zip(values, branches)]

        # Do these last to make the blocks' output order nicer.
        default_builder = BlockBuilder(self._label(tag, "default"), self._fbuilder) if not default else default
        cont_builder = BlockBuilder(self._label(tag, "cont"), self._fbuilder) if not cont else cont

        if not default:
            default_builder.block().setNext(cont_builder._block.name())
        
        for b in branches:
            b.block().setNext(cont_builder._block.name())
        
        self.switch(cond, default_builder.labelOp(), self.tupleOp(alts))
            
        return (default_builder, branches, cont_builder)

    def makeInternalError(self, msg):
        """Generates an internal_error instruction.
        
        msg: string - A message to associate with the error.
        """
        self.debug_internal_error(self.constOp(msg))
        
    def makeDebugMsg(self, stream, msg, args=[]):
        """Generates a ~~debug.Msg instruction.
    
        stream: string - The name of the debug stream.
        msg: string - The format string.
        args: optional list of ~~Operand - The list of format arguments.
        """
        msg = hilti.instructions.debug.message(stream, msg, args)
        self._block.addInstruction(msg)
    
    def makeRaiseException(self, exception, arg):
        """Raises an exception.
        
        exception: string - The name of the exception.
        arg: ~~Operand or None - The argument for the exception, or none if
        the exception type does not expect an argument.
        """
        etype = self.idOp(exception)
        util.check_class(etype.type(), type.Exception, "Builder.makeRaiseException")
        
        def _addExcptLocal():
            ename = self._fbuilder._idName("excpt")
            return self._fbuilder.addLocal(ename, type.Reference(etype.type()))
        
        local = self._fbuilder.cache("makeRaiseException_local", _addExcptLocal)
        
        self.new(local, etype, arg)
        self.exception_throw(local)
        
def _init_builder_instructions():
    # Adds all instructions as methods to the Builder class.
    for (name, ins) in instruction.getInstructions().items():
        
        def _getop(args, sig, op):
            if not sig:
                return (None, args)
            
            if not args and "_optional" in sig.__dict__:
                return (None, args)
        
            if not args:
                raise IndexError(op)
            
            assert (not args[0]) or isinstance(args[0], operand.Operand)
            
            return (args[0], args[1:])
        
        def _make_build_function(name, ins, sig):
            def _do_build(self, *args):
                try:
                    (target, args) = _getop(args, sig.target(), "target")
                    (op1, args) = _getop(args, sig.op1(), "operand 1")
                    (op2, args) = _getop(args, sig.op2(), "operand 2")
                    (op3, args) = _getop(args, sig.op3(), "operand 3")
                    
                    if args:
                        raise TypeError("too many arguments for instruction %s" % _do_build.__name__)
                    
                except IndexError, e:
                    raise TypeError("%s missing for instruction %s" % (e, _do_build.__name__))
                
                ins = _do_build._ins(op1=op1, op2=op2, op3=op3, target=target)
                ins.setComment(self._next_comment)
                self._next_comment = []
                
                self._block.addInstruction(ins)
                
                if sig.target():
                    return target
                
            _do_build.__name__ = name
            _do_build._ins = ins
            return _do_build
            
        name = name.replace(".", "_")
        
        if name in ("yield"):
            # Work-around reserved Python keywords.
            name = name + "_"
        
        setattr(BlockBuilder, name, _make_build_function(name, ins, ins._signature))
    
_init_builder_instructions()    
    
    
    
    


# $Id$

import ast
import instruction
import function
import id
import block
import type
import constant
import module

class ModuleBuilder(object):
    """A class for adding entities to a module.
    
    module: Module - The model going to be extended.
    """
    def __init__(self, module):
        self._module = module

    def module(self):
        """Returns the module associated with this builder.
        
        Returns: Module - The module.
        """
        return self._module
    
    def addConstant(self, name, type, value):
        """Adds a new global constant to the module scope's.
        
        name: string - The name of the global ID. 
        type: HiltiType - The type of the global ID.
        value: any - The value the constant; it's type must correspond to whatever *type* demands. 
        
        Returns: ~~IDOperand - An ID operand referencing the constant. 
        """
        i = id.ID(name, type, id.Role.CONST)
        value = self.constOp(value, type)
        self._module.addID(i, value)
        return instruction.IDOperand(i)
        
    def addTypeDecl(self, name, ty):
        """Adds a new global type declaration to the module scope's.
        
        name: string - The name of the global ID for the type declaration. 
        ty: HiltiType - The type being declared. 
        
        Returns: ~~TypeOperand - A type operand referencing the declaration. 
        """
        i = id.ID(name, type.TypeDeclType(ty), id.Role.GLOBAL)
        self._module.addID(i)        
        return self.typeOp(ty)

    def constOp(self, value, type):
        """Return a constant operand.
        
        value: any - The value of the constant.
        type: HiltiType - The type of the constant.
        
        Returns: ~~ConstOperand - The constant operand.
        """
        return instruction.ConstOperand(constant.Constant(value, type))
    
    def typeOp(self, type):
        """Return a type operand.
        
        type: HiltiType - The type reference by the operand.
        
        Returns: ~~TypeOperand - The type operand.
        """
        return instruction.TypeOperand(type)
        
class BlockBuilder(ModuleBuilder): 
    """A class for building sequences of instructions in memory. Each instance
    of a Builder is associated with a ~~Block to which the instruction will be
    added. 
    
    A ~~Builder supports all HILTI instructions as method calls. In general,
    an instruction of the form ``<target> = <part1>.<part2> <op1> <op2>``
    translated into a builder method ``<part1>_<part2>(<target>, <op1>,
    <op2>)`` where ``<target>`` and ``<op1>/<op2>`` are instances of
    ~~instruction.Operand. If an instruction has target, the method returns
    the target operand. (Note that these methods are not documented in the
    ~~Builder's method reference.)

    mod: Module - The module the block is part of.
    b: Block - The block the builder adds instruction to. 
    """
    def __init__(self, mod, b):
        assert isinstance(mod, module.Module)
        assert isinstance(b, block.Block)
        super(BlockBuilder, self).__init__(mod)
        self._block = b
        
    def block(self):
        """Returns the block this builder is adding instructions to.
        
        Returns: Block - The block.
        """
        return self._block

    def _setBlock(self, b):
        assert isinstance(b, block.Block)
        self._block = b

class FunctionBuilder(BlockBuilder):
    """A class for building functions. Instructions can be added to the
    function in the same way as provided by ~~Builder for blocks.
    
    mbuilder: ModuleBuilder - The builder for the module the function will become part of.
    name: string - The name of the function.
    ftype: type.Function - The type of the function.
    cc: function.CallingConvention - The function's calling convention.
    location: Location - The location to be associated with the function.
    """
    def __init__(self, mbuilder, name, ftype, cc=function.CallingConvention.HILTI, location=None):
        i = id.ID(name, ftype, id.Role.GLOBAL)
        self._func = function.Function(ftype, i, cc, location)
        self._mbuilder = mbuilder
        mbuilder.module().addID(i, self._func)
        self._block = block.Block(self._func)
        self._func.addBlock(self._block)
        self._blocks = 1
        super(FunctionBuilder, self).__init__(mbuilder.module(), self._block)
        
    def function(self):
        """Returns the HILTI function being build.
        
        Returns: Function - The function.
        """
        return self._func

    def idOp(self, i):
        """Return an ID operand.
        
        i: ID or string - The ID, or the name of the ID which is then
        looked up in the current scope. In the latter case, the ID *must*
        exist. 
        
        Returns: IDOperand - The ID operand.
        """
        if isinstance(i, str):
            li = self._func.lookupID(i)
            if not li:
                li = self._mbuilder.module().lookupID(i)
            if not li:
                raise ValueError("ID %s does not exist in function's scope" % i)
        
            i = li
            
        return instruction.IDOperand(i)

    def addLocal(self, name, type, value = None):
        """Adds a new local variable. 
        
        name: string - The name of the variable.
        type: ValueType - The type of the variable.
        value: ConstOperand - Optional initialization of the local. 
        
        Returns: IDOperand - An IDOperand referencing the new local. 
        """
        i = id.ID(name, type, id.Role.LOCAL)
        self._func.addID(i)
        
        op = self.idOp(i)
        
        if value:
            assert isinstance(value, instruction.ConstOperand)
            self.assign(op, self.constOp(value, type))
            
        return op
    
    def startNewBlock(self, postfix = None, _block=None):
        """Starts a new block.
        
        postfix: string - A label appended to the auto-generated block name.
        """
        self._block = block.Block(self._func, name=self._blockName(postfix))
        self._func.addBlock(self._block)
        self._setBlock(self._block)
        
    def makeIfElse(self, cond):
        """Builds an If-Else construct based on a given condition. The
        function returns two ~~BlockBuilder for the two branches, which can
        then be filled. It also starts a new block within this FunctionBuilder
        to which all subsequent instruction will be added. The two branches
        continue execution with that block eventually (unless they leave the
        function otherwise). 
        
        cond: Operand - The condition operand; must be of type bool.
        
        Returns (BlockBuilder, BlockBuilder) - The first builder is for the
        *true* branch, the second for the *false* branch.
        """
        assert cond.type() == type.Bool
        
        yes = BlockBuilder(self._mbuilder.module(), block.Block(self._func, name=self._blockName("true")))
        no = BlockBuilder(self._mbuilder.module(), block.Block(self._func, name=self._blockName("false")))
        self._func.addBlock(yes.block())
        self._func.addBlock(no.block())
        
        self.if_else(cond, self.idOp(yes.block().name()), self.idOp(no.block().name()))
        
        self.startNewBlock("cont")
        yes.block().setNext(self._block)
        no.block().setNext(self._block)
        
        return (yes, no)
    
    def _makeIf(self, cond, case):
        assert cond.type() == type.Bool

        postfix = "true" if case else "false"
        branch = BlockBuilder(self._mbuilder.module(), block.Block(self._func, name=self._blockName(postfix)))
        self._func.addBlock(branch.block())

        cont = block.Block(self._func, name=self._blockName("cont"))
        self._func.addBlock(cont)
        
        op1 = self.idOp(branch.block().name())
        op2 = self.idOp(cont.name())
        
        if case:
            self.if_else(cond, op1, op2)
        else:
            self.if_else(cond, op2, op1)
        
        self._setBlock(cont)
        branch.block().setNext(self._block)
        
        return branch
    
    def makeIf(self, cond):
        """Builds an If construct based on a given condition. The function
        returns one ~~BlockBuilder for the case that the condition is true,
        which can then be filled. It also starts a new block within this
        FunctionBuilder to which all subsequent instructions will be added.
        This block will be immediately branched to if the condition is false.
        The true-branch continues execution with that block eventually as well
        (unless it leaves the function otherwise first).
        
        cond: Operand - The condition operand; must be of type bool.
        
        Returns BlockBuilder - The builder for the *true* branch. 
        """
        return self._makeIf(cond, True)
    
    def makeIfNot(self, cond):
        """Builds an If construct based on a given condition. The function
        returns one ~~BlockBuilder for the case that the condition is true,
        which can then be filled. It also starts a new block within this
        FunctionBuilder to which all subsequent instructions will be added.
        This block will be immediately branched to if the condition is false.
        The true-branch continues execution with that block eventually as well
        (unless it leaves the function otherwise first).
        
        cond: Operand - The condition operand; must be of type bool.
        
        Returns BlockBuilder - The builder for the *true* branch. 
        """
        return self._makeIf(cond, False)
            
    def _blockName(self, postfix=""):
        self._blocks += 1
        return "b%d%s" % (self._blocks, ("_%s" % postfix) if postfix else "")
        
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
            
            assert isinstance(args[0], instruction.Operand)
            
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
    
    
    
    


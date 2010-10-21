# $Id$

builtin_id = id

import constant
import node
import id
import operand
import type

import hilti.instructions.debug

class Block(node.Node): 
    """Groups a series of instructions that are to be executed in sequence.
    Multiple blocks can be chained to indicate where control-flow is
    transfered to when a block's last instruction has been executed (assuming
    it did not explictly transfer control-flow otherwise).

    function: ~~Function - The parent function the block is a part of. 
    
    may_remove: bool - If True, the ~~canonifier is allowed to
    remove this block of it's empty.
    
    instructions: list of ~~Instruction - The initial list of
    instructions.
    
    name: string - An optional name for the block, to be used in a 
    ~~Label. If the name is not given, a unique one will be created.

    location: ~~Location - A location to be associated with the block. 
    """

    _label_cnt = 1
    
    def __init__(self, function, may_remove=False, instructions = None, name = None, location = None):
        super(Block, self).__init__(location)
        self._function = function
        self._ins = instructions if instructions else []
        self._next = None
        self._may_remove = may_remove
        self._name_set = False

        if name:
            self._name = name
        else:
            self._name = "@__b%d" % Block._label_cnt
            Block._label_cnt += 1
        
    def function(self):
        """Returns the block's parent function. 
        
        Returns: ~~Function - The parent.
        """
        return self._function
        
    def instructions(self):
        """Returns the block's instructions.
        
        Returns: list of ~~Instruction - The instructions.
        """
        return self._ins

    def addInstruction(self, ins):
        """Adds an instruction to the block. The instruction is appended to
        the current list of instructions. 
        
        ins: ~~Instruction - The instruction to add.
        """
        self._ins += [ins]
        
    def addInstructionAtFront(self, ins):
        """Adds an instruction to the block. The instruction is appended to
        the current list of instructions. 
        
        ins: ~~Instruction - The instruction to add.
        """
        self._ins = [ins] + self._ins

    def replace(self, other):
        """Replaces the block's content with that of another block. It
        replaces the block's instructions, its name and its location
        information, allowing for in-place modification. The method does not
        change the results of :meth:`function`, :meth:`next`, and
        :meth:`mayRemove`.
        
        other: The block to take the content from. The other block is left
        untouched.
        """
        self._ins = other._ins
        self._name = other._name
        self._location = other._location
    
    def name(self):
        """Returns the name of the block. 
        
        Returns: string - The name. Every block has a name. 
        """
        assert self._name
        return self._name

    def setName(self, name):
        """Sets the name of the block.
        
        name: string - The name of the block.
        """
        assert name
        self._name = name
    
    def next(self):
        """Returns the name of the block's successor. Blocks are linked.
        
        Returns: string - The block's successor.
        """
        return self._next
    
    def setNext(self, succ):
        """Sets the name of th Block's successor. 
        
        succ: string: The new successor. Can be None for none.
        """
        assert not succ or isinstance(succ, str)
        self._next = succ
    
    def setMayRemove(self, may):
        """Indicates whether the block can be removed from a function if empty. 
        
        may: bool: True if removal is permitted.
        """
        self._may_remove = may
        
    def mayRemove(self):
        """Queries whether whe block can be removed from a function if empty. 
        
        Returns: bool - If True, block may be removed.
        """
        return self._may_remove

    def __str__(self):
        name = self._name if self._name else "<anonymous-block>"
        next = " (next: %s)" % self._next if self._next else "(next not set)"
        return "block <%s>%s\n" % (name, next)

    ### Overridden from Node.

    def _resolve(self, resolver):
        for i in self._ins:
            i.resolve(resolver)
    
    def _validate(self, vld):
        for i in self._ins:
            i.validate(vld)
    
    def output(self, printer):
        if self._name:
            printer.pop()
            printer.output("%s:" % self._name, nl=True)
            printer.push()

        printer.printComment(self)
            
        for ins in self._ins:
            ins.output(printer)
            
        if self._next and (not len(self._ins) or not self._ins[-1].signature().terminator()):
            # Debugging aid: show the implicit jump at the end of a block that
            # has not terminator instruction there.
            printer.output("# jump %s" % self._next, nl=True)

    ### Overridden from Node.
        
    def _canonify(self, canonifier):
        # If we are in debug mode, first add message instructions. 
        if canonifier.debugMode():
            b = Block(canonifier.currentFunction(), instructions=[], name=self._name, location=self.location())
            b.setMayRemove(self.mayRemove())
            b.setNext(self.next())
            
            for i in self._ins:
                _addDebugPreInstruction(canonifier, b, i)
                b.addInstruction(i)
                _addDebugPostInstruction(canonifier, b, i)
                
        else:
            b = self
        
        # While we proceed, we copy each instruction over to a new
        # block,  potentially after transforming it first.
        new_block = Block(canonifier.currentFunction(), instructions=[], name=b._name, location=b.location())
        new_block.setMayRemove(b.mayRemove())
        new_block.setNext(b.next())
        canonifier.addTransformedBlock(new_block)
        
        for i in b._ins:
            canonifier.setCurrentInstruction(i)
        
            i.canonify(canonifier)
            
            ni = canonifier.currentInstruction()
            if ni:
                canonifier.currentTransformedBlock().addInstruction(ni)
    
        # If the original block was linked to a successor, but the last addedd
        # transformed is not, copy that link over. 
        if b.next() and not canonifier.currentTransformedBlock().next():
            canonifier.currentTransformedBlock().setNext(b.next())
                
    def _codegen(self, cg):
        cg.startBlock(self)
            
        for i in self._ins:
            cg.startInstruction(i)
            i.codegen(cg)
            cg.endInstruction()
        
        cg.endBlock()
    
    # Visitor support.
    def visit(self, visitor):
        visitor.visitPre(self)
        
        for ins in self._ins:
            ins.visit(visitor)
            
        visitor.visitPost(self)

def _addDebugPreInstruction(canonifier, b, i):
    if isinstance(i, hilti.instructions.debug.Msg):
        return
    
    def replace(op):
        if isinstance(op.type(), type.Function):
            return operand.Constant(constant.Constant(op.value().name(), type.String()))
        
        if isinstance(op.type(), type.Label):
            return operand.Constant(constant.Constant(op.value().name(), type.String()))
        
        if isinstance(op.type(), type.Tuple):
            return operand.Constant(constant.Constant("tuple", type.String())) # XXX
        
            if isinstance(op, operand.Constant):
                tup = [replace(o) for o in op.value().value()]
                return operand.Constant(constant.Constant(tup, type.Tuple([e.type() for e in tup])))
            else:
                return operand.Constant(constant.Constant("tuple", type.String()))
        
        if isinstance(op, operand.Type):
            return operand.Constant(constant.Constant(str(op), type.String()))
        
        if isinstance(op, operand.Ctor):
            return operand.Constant(constant.Constant(str(op), type.String()))

        if isinstance(op, operand.ID):
            return operand.Constant(constant.Constant(op.value().name(), type.String()))
            
        if not isinstance(op.type(), type.HiltiType):
            return operand.Constant(constant.Constant("<%s>" % op.type().name(), type.String()))

        return op
    
    args = [replace(op) for op in (i.op1(), i.op2(), i.op3()) if op]
    fmt = ["%s"] * len(args)
    dbg = hilti.instructions.debug.message("hilti-trace", "%s %s %s" % (i.location(), i.name(), " ".join(fmt)), args)
    b.addInstruction(dbg)

def _addDebugPostInstruction(canonifier, b, i):
    if isinstance(i, hilti.instructions.debug.Msg):
        return
    
    if not i.target():
        return 
    
    dbg = hilti.instructions.debug.message("hilti-trace", "%s -> %%s" % i.location(), [i.target()])
    b.addInstruction(dbg)

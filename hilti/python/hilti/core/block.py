# $Id$

import ast
import location

class Block(ast.Node): 
    """Groups a series of instructions that are to be executed in sequence.
    Multiple blocks can be chained to indicate where control-flow is
    transfered to when a block's last instruction has been executed (assuming
    it did not explictly transfer control-flow otherwise).

    function: ~~Function - The parent function the block is a part of. 
    
    may_remove: bool - If True, the ~~canonifier is allowed to
    remove this block of it's empty.
    
    instructions: list of ~~Instruction - The initial list of
    instructions.
    
    name: string - An optional name for the block, to be used in
    ~~Labels. 

    location: ~~Location - A location to be associated with the block. 
    """
    
    def __init__(self, function, may_remove=True, instructions = None, name = None, location = None):
        super(Block, self).__init__(location)
        self._function = function
        self._ins = instructions if instructions else []
        self._name = name
        self._next = None
        self._may_remove = may_remove

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
        """Returns the name of the block 
        
        Returns: string - The name.
        """
        return self._name

    def setName(self, name):
        """Sets the name of the block.
        
        name: string - The name of the block.
        """
        self._name = name
    
    def next(self):
        """Returns the block's successor. Blocks are linked        
        
        Returns: ~~Block - The block's successor.
        """
        return self._next
    
    def setNext(self, succ):
        """Sets the Block's successor. 
        
        succ: ~~Block: The new successor.
        """
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
        next = " (next: %s)" % self._next.name() if self._next else "(next not set)"
        return "block <%s>%s\n" % (name, next)
    
    # Visitor support.
    def visit(self, visitor):
        visitor.visitPre(self)
        
        for ins in self._ins:
            ins.visit(visitor)
            
        visitor.visitPost(self)
        

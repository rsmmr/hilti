# $Id$

import ast
import location

class Block(ast.Node): 
    """A Block models a series of instructions that is executed in sequence. 
    
    A *well-formed* Block is a Block with exactly one |terminator|,
    and this terminator must be the very last instruction. A Block
    can be not well-formed during construction, and the
    ~~canonifier will eventually turn it into a
    well-formed one.
    """
    
    def __init__(self, function, may_remove=True, instructions =
        None, name = None, location = None):
        """Initializes a Block. 
        
        The *function* is the parent
        ~~Function this Block is a part
        of. *may_remove* is a boolen indicating whether
        ~~canonifier is allowed to remove this Block if
        it's empty. *instructions* is the initial list of
        ~~Instruction objects. The
        Block is optionally given a string *name*, and a *location*
        as a ~~Location."""
        
        self._function = function
        self._ins = instructions if instructions else []
        self._name = name
        self._location = location
        self._next = None
        self._may_remove = may_remove

    def function(self):
        """(A) Returns the parent ~~Function of this Block.
        
        (B) Returns the parent :class:`~hilti.core.function.Function` of this Block.
        """
        return self._function
        
    def instructions(self):
        """Returns the list of
        ~~Instruction objects forming
        this Block."""
        return self._ins

    def addInstruction(self, ins):
        """Adds an ~~Instruction
        *ins* to the end of the current list."""
        self._ins += [ins]

    def replace(self, other):
        """Replaces the instruction sequence of this Block, as well as the
        name and the location information, with that of the given Block. This
        allows for an in-place substitution. Does not change the results of 
        :meth:`function`, :meth:`next`, and :meth:`mayRemove`."""
        self._ins = other._ins
        self._name = other._name
        self._location = other._location
    
    def name(self):
        """Returns the name of this Block as a string."""
        return self._name

    def setName(self, name):
        """Sets the name of this Block to the string *name*."""
        self._name = name
    
    def next(self):
        """Returns this Block's successor Block. The successor is the Block
        where control is transfered to after executing the last instruction
        (if the last instruction is a a flow-control instruction, that takes
        precedent)."""
        return self._next
    
    def setNext(self, b):
        """Sets this Block's successor Block to *b*. The successor is the
        Block where control is transfered to after executing the last
        instruction (if the last instruction is a a flow-control instruction,
        that takes precedent)."""
        self._next = b
    
    def location(self):
        """Returns the ~~Location object for this
        Block."""
        return self._location

    def setMayRemove(self, may):
        """If the boolean *may* is true, the ~~canonifier is
        allowed to remove this Block in case it does not have any
        instructions. The default is True."""
        self._may_remove = may
        
    def mayRemove(self):
        """Returns a boolean indicating whether the ~~canonifier
        is allowed to remove this Block in case it does not have any
        instructions."""
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
        

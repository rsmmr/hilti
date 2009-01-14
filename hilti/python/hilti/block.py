# $Id$
#
# A block is a series of instructions which are executed in sequence (i.e., static
# control flow).

import location

class Block(object):
    def __init__(self, instructions = None, name = None, location = location.Location()):
        self._ins = instructions if instructions else []
        self._name = name
        self._location = location
        self._next = None

    def instructions(self):
        return self._ins

    def addInstruction(self, ins):
        self._ins += [ins]

    # Replaces content of this block with argument in place.
    def replace(self, other):
        self._ins = other._ins
        self._name = other._name
        self._location = other._location
    
    def name(self):
        return self._name

    def setName(self, name):
        self._name = name
    
    def next(self):
        return self._next
    
    def setNext(self, b):
        self._next = b
    
    def location(self):
        return self._location
    
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        
        for ins in self._ins:
            ins.dispatch(visitor)
            
        visitor.visit_post(self)
        
    def __str__(self):
        name = self._name if self._name else "<anonymous-block>"
        next = " (next: %s)" % self._next.name() if self._next else "(next not set)"
        return "block <%s>%s\n" % (name, next)

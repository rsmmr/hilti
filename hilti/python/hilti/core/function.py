# $Id$
#
# A function. 

import location
import scope

class Function(object):

    # Linkage.  
    LINK_PRIVATE = 1 
    LINK_EXTERN = 2
    
    # Calling conventions.
    CC_HILTI = 1
    CC_C = 2
    
    def __init__(self, name, type, location = location.Location(), parentfunc = None):
        self._name = name
        self._type = type
        self._bodies = []
        self._location = location
        self._scope = scope.Scope()
        self._parent = parentfunc
        self._linkage = self.LINK_PRIVATE
        self._cc = self.CC_HILTI
        
        if parentfunc:
            self._scope = parentfunc._scope

    def name(self):
        return self._name
        
    def type(self):
        return self._type

    def linkage(self):
        return self._linkage
    
    def callingConvention(self):
        return self._cc

    def setLinkage(self, linkage):
        self._linkage = linkage
    
    def setCallingConvention(self, cc):
        self._cc = cc
    
    def scope(self):
        return self._scope
    
    def addID(self, id):
        self._scope.insert(id)
        
    def addBlock(self, b, clear_next=True):
        if self._bodies:
            self._bodies[-1].setNext(b)
            
        self._bodies += [b] 
        if clear_next:
            b.setNext(None)
    
    def blocks(self):
        return self._bodies

    def clearBlocks(self):
        self._bodies = []

    def lookupBlock(self, id):
        for b in self._bodies:
            if b.name() == id.name():
                return b
        return None
        
    def hasImplementation(self):
        return len(self._bodies) > 0
    
    def parentFunc(self):
        return self._parent
        
    def location(self):
        return self._location

    def __str__(self):
        return "function %s %s" % (self._type, self._name)
    
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        self._scope.dispatch(visitor)
        for b in self._bodies:
            b.dispatch(visitor)
        visitor.visit_post(self)
        
    
    

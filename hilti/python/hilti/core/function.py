# $Id$
#
# A function. 

import location
import scope

class Function(object):

    # Linkage.  
    LINK_LOCAL = 1    # module-wide
    LINK_EXPORTED = 2 # across compilation units
    
    # Calling conventions.
    CC_HILTI = 1
    CC_C = 2
    
    def __init__(self, name, type, module, parentfunc = None, location = location.Location()):

        assert name.find("::") < 0 # must be non-qualified

        if module:
            self._name = "%s::%s" % (module.name(), name)
        else:
            self._name = "(private)::%s" % name 
            
        self._type = type
        self._bodies = []
        self._location = location
        self._scope = scope.Scope()
        self._parent = parentfunc
        self._linkage = self.LINK_LOCAL
        self._cc = self.CC_HILTI
        self._module = module
        self._is_imported = False
        
        if parentfunc:
            self._scope = parentfunc._scope

    def name(self):
        return self._name
        
    def type(self):
        return self._type

    # Returns the module that the function was *defined in*. Note
    # that a function can be # in the scope of other modules as well
    # if they imported it.
    def module(self): 
        return self._module
    
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
    
    def setImported(self, is_imported):
        self._is_imported = is_imported
    
    def isImported(self):
        return self._is_imported
    
    def parentFunc(self):
        return self._parent
        
    def location(self):
        if self._location:
            return self._location
        
        if self._parentfunc:
            return self._parentfunc.location()
        
        return None

    def __str__(self):
        return "function %s %s" % (self._type, self._name)
    
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        self._scope.dispatch(visitor)
        for b in self._bodies:
            b.dispatch(visitor)
        visitor.visit_post(self)
        
    
    

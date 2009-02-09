# $Id$

import location
import scope
import type

class Linkage:
    """The *linkage* of a ~~Function specifies its
    link-time visibility."""
    
    LOCAL = 1    
    """A ~~Function with linkage :const:`LOCAL` is
    only visible inside the ~~Module it is
    implemented in. This is the default linkage."""

    EXPORTED = 2
    """A ~~Function with linkage :const:`EXPORTED`
    is visible across all compilation units."""
    
    
class CallingConvention:
    """The *calling convention* used by a
    ~~Function specifies how calls to it are
    implemented."""
    
    HILTI = 1
    """A ~~Function using the proprietary HILTI
    calling convention supports all of HILTI's flow-control features but
    cannot be called from other languages such as C. This is the default
    calling convention."""
    
    C = 2
    """A ~~Function using C calling convention
    implements standard C call semantics. It does however not support any
    HILTI-specific flow-control features, such as exceptions and
    continuations.
    
    .. note:
    Currently, functions with C calling convention cannot be *implemented* in
    HILTI but must be defined in an external, separately compiled object file.
    They can be *called* from HILTI programs however. In the future, it will also be
    possible to define functions with C calling convention in HILTI itself,
    which will then be callable *from* C programs. 
    """
        
class Function(object):
    """A Function object represents all functions either declared or defined
    in a ~~Module. 
    
    *name* is a string containing the name of the Function; the name must be
    *non-qualified*, i.e., not be prefixed with any module name.  *ty* is the
    ~~FunctionType of this Function. *module* is the
    ~~Module that provides the Function's
    *implementation*.  For a Function with linkage :const:`Linkage.C`,
    *module* may be *None*. *location* defines a
    ~~Location to be associated with this
    Function.
    
    This class implements ~~Visitor support
    without subtypes. 
    """
    
    def __init__(self, name, ty, module, cc=CallingConvention.HILTI, location=None):
        assert name
        assert name.find("::") < 0 # must be non-qualified
        assert module or cc == CallingConvention.HILTI
        assert isinstance(ty, type.FunctionType)
        
        if module:
            self._name = "%s::%s" % (module.name(), name)
        else:
            self._name = name 
            
        self._type = ty
        self._bodies = []
        self._location = location
        self._linkage = Linkage.LOCAL
        self._cc = cc
        self._module = module
        
        self._scope = {}
        for id in ty.IDs():
            self._scope[id.name()] = id 

    def name(self):
        """Returns the fully qualified name of this Function as a string."""
        return self._name
        
    def type(self):
        """Returns the ~~FunctionType of this
        function.""" 
        return self._type

    def module(self): 
        """Returns the ~~Module that provides the
        *implementation* of the function. Returns *None* if the Function has
        :const:`Linkage.C`."""
        return self._module
    
    def linkage(self):
        """Returns the :class:`Linkage` of this Function."""
        return self._linkage
    
    def callingConvention(self):
    	"""Returns the :class:`CallingConvention` of this Function."""
        return self._cc

    def setLinkage(self, linkage):
        """Sets the :class:`Linkage` of this Function."""
        self._linkage = linkage
    
    def setCallingConvention(self, cc):
    	"""Sets the :class:`CallingConvention` of this Function."""
        self._cc = cc
        
        if cc == CallingConvention.C:
            self._module = None
        else:
            # Technically, I guess we should have a way to set a module when
            # switching to CC HILTI. Don't seem to happen at the moment though. 
            assert self._module
    
    def addID(self, id):
        """Inserts an ~~ID *id* into the function's scope. *id* must not be
        qualified."""
        assert not id.qualified()
        self._scope[id.name()] = id 
        
    def lookupID(self, name):
        """Returns the ~~ID having the given *name* if
        it is defined in the Function's local scope, or None if not."""
        try:
            return self._scope[name]
        except KeyError:
            return None
    
    def IDs(self):
        """Returns a list of all ~~ID objects defined in
        the Function's scope."""
        return self._scope.values()
        
    def addBlock(self, b):
        """Appends a ~~Block to the Function's
        current list of blocks. The new Block is set to be the successor of
        the former tail Block, and its own successor field is cleared."""
        if self._bodies:
            self._bodies[-1].setNext(b)
            
        self._bodies += [b] 
        b.setNext(None)
    
    def blocks(self):
        """Returns the list of ~~Block objects
        defining the Function's implementation. This list may be empty."""
        return self._bodies

    def clearBlocks(self):
        """Removes all ~~Block object from this
        Function."""
        self._bodies = []

    def lookupBlock(self, id):
        """Returns the ~~Block with the same name as
        the ~~ID *id*, or None if there is no such
        Block."""
        for b in self._bodies:
            if b.name() == id.name():
                return b
        return None
    
    def location(self):
        """Returns the ~~Location associated with
        this Function."""
        return self._location

    def __str__(self):
        return "function %s %s" % (self._type, self._name)
    
    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit_pre(self)
        
        for id in self._scope.values():
            id.dispatch(visitor)
        
        for b in self._bodies:
            b.dispatch(visitor)
            
        visitor.visit_post(self)
        
    
    

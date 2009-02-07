# $Id$
#
# A function. 

import location
import scope
import type

class Linkage:
    """The *linkage* of a :class:`~hilti.core.function.Function` specifies its
    link-time visibility."""
    
    LOCAL = 1    
    """A :class:`~hilti.core.function.Function` with linkage :const:`LOCAL` is
    only visible inside the :class:`~hilti.core.module.Module` it is
    implemented in. This is the default linkage."""

    EXPORTED = 2
    """A :class:`~hilti.core.function.Function` with linkage :const:`EXPORTED`
    is visible across all compilation units."""
    
    
class CallingConvention:
    """The *calling convention* used by a
    :class:`~hilti.core.function.Function` specifies how calls to it are
    implemented."""
    
    HILTI = 1
    """A :class:`~hilti.core.function.Function` using the proprietary HILTI
    calling convention supports all of HILTI's flow-control features but
    cannot be called from other languages such as C. This is the default
    calling convention."""
    
    C = 2
    """A :class:`~hilti.core.function.Function` using C calling convention
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
    in a :class:`~hilti.core.module.Module`. 
    
    *name* is a string containing the name of the Function; the name must be
    *non-qualified*, i.e., not be prefixed with any module name.  *ty* is the
    :class:`~hilti.core.type.FunctionType` of this Function. *module* is the
    :class:`~hilti.core.module.Module` that provides the Function's
    *implementation*.  For a Function with linkage :const:`Linkage.C`,
    *module* may be *None*. *location* defines a
    :class:`~hilti.core.location.Location` to be associated with this
    Function.
    
    This class implements :class:`~hilti.core.visitor.Visitor` support
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
        self._scope = scope.Scope()
        self._linkage = Linkage.LOCAL
        self._cc = cc
        self._module = module
        
        for id in ty.IDs():
            self._scope.insert(id)

    def name(self):
        """Returns the fully qualified name of this Function as a string."""
        return self._name
        
    def type(self):
        """Returns the :class:`~hilti.core.type.FunctionType` of this
        function.""" 
        return self._type

    def module(self): 
        """Returns the :class:`~hilti.core.module.Module` that provides the
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
    
#    def scope(self):
#        """Returns the local :class:`~hilti.core.scope.Scope` of this
#        Function. The Scope contains Function parameters and local variables."""
#        return self._scope
    
    def addID(self, id):
        """Inserts an :class:`~hilti.core.id.ID` into the function's scope."""
        self._scope.insert(id)
        
    def lookupID(self, name):
        """Returns the :class:`~hilti.core.id.ID` having the given *name* if
        it is defined in the Function's local scope, or None if not."""
        return self._scope.lookup(name)
    
    def IDs(self):
        """Returns a list of all :class:`~hilti.core.id.ID` objects defined in
        the Function's scope."""
        return self._scope.IDs()
        
    def addBlock(self, b):
        """Appends a :class:`~hilti.core.block.Block` to the Function's
        current list of blocks. The new Block is set to be the successor of
        the former tail Block, and its own successor field is cleared."""
        if self._bodies:
            self._bodies[-1].setNext(b)
            
        self._bodies += [b] 
        b.setNext(None)
    
    def blocks(self):
        """Returns the list of :class:`~hilti.core.block.Block` objects
        defining the Function's implementation. This list may be empty."""
        return self._bodies

    def clearBlocks(self):
        """Removes all :class:`~hilti.core.block.Block` object from this
        Function."""
        self._bodies = []

    def lookupBlock(self, id):
        """Returns the :class:`~hilti.core.block.Block` with the same name as
        the :class:`~hilti.core.id.ID` *id*, or None if there is no such
        Block."""
        for b in self._bodies:
            if b.name() == id.name():
                return b
        return None
    
    def location(self):
        """Returns the :class:`~hilti.core.location.Location` associated with
        this Function."""
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
        
    
    

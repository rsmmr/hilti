# $Id$

import ast
import location
import type
import id

class Linkage:
    """The *linkage* of a ~~Function specifies its link-time
    visibility."""
    
    LOCAL = 1    
    """A ~~Function with linkage LOCAL is only visible inside the
    ~~Module it is implemented in. This is the default linkage."""

    EXPORTED = 2
    """A ~~Function with linkage EXPORTED is visible across all
    compilation units."""
        
class CallingConvention:
    """The *calling convention* used by a ~~Function specifies which
    implementation calls to it use."""
    
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
    
    Note: Currently, functions with C calling convention cannot be
    *implemented* in HILTI but must be defined in an external,
    separately compiled object file. They can be *called* from HILTI
    programs however. In the future, it will also be possible to
    define functions with C calling convention in HILTI itself,
    which will then be callable *from* C programs. 
    """
        
class Function(ast.Node):
    """A HILTI function declared or defined inside a module.
    
    name: string - Name of the function. It must be *non-qualified*,
    i.e., not be prefixed with any module name.
    
    ty: :class:`~hilti.core.type.Function` - Type of the function. 
      
    module: ~~Module - Module that provides the function's
    *implementation*. For a Function with linkage ~~C, *module* can
    be None.
    
    location: ~~Location - Location to be associated with the
    function. 
    """
    
    def __init__(self, name, ty, module, cc=CallingConvention.HILTI, location=None):
        assert name
        assert name.find("::") < 0 # must be non-qualified
        assert module or cc == CallingConvention.HILTI
        assert isinstance(ty, type.Function)
        
        if module:
            self._name = "%s::%s" % (module.name(), name)
        else:
            self._name = name 
            
        self._type = ty
        self._bodies = []
        self._location = location
        # Implictly export the main function.
        self._linkage = Linkage.LOCAL if self._name.lower() != "main::run" else Linkage.EXPORTED
        self._cc = cc
        self._module = module
        
        self._scope = {}
        for id in ty.Args():
            self._scope[id.name()] = id 

    def name(self):
        """Returns the function's name.
        
        Returns: string - The fully-qualified name of the function.
        """
        return self._name
        
    def type(self):
        """Returns the type of the function. 
        
        Returns: :class:`~hilti.core.type.Function` - The type of the
        function.
        """
        return self._type

    def module(self): 
        """Returns the function's module. 
        
        Returns: ~~Module - The module that provides the implementation of the
        function. Returns None if the function has ~~Linkage ~~C. 
        """
        return self._module
    
    def linkage(self):
        """Returns the linkage of the function.
        
        Returns: ~~Linkage - The linkage of the function."""
        return self._linkage
    
    def callingConvention(self):
        """Returns the calling convention used by the function.
        
        Returns: ~~CallingConvention - The calling convention used by the
        function."""
        return self._cc

    def setLinkage(self, linkage):
        """Sets the function's linkage.
        
        linkage: ~~Linkage - The linkage to set.
        """
        self._linkage = linkage
    
    def setCallingConvention(self, cc):
        """Sets the function's calling convention.
        
        cc: ~~CallingConvention - The calling convention to set.
        """
        self._cc = cc
        
        if cc == CallingConvention.C:
            self._module = None
        else:
            # Technically, I guess we should have a way to set a module when
            # switching to CC HILTI. Don't seem to happen at the moment though. 
            assert self._module
    
    def addID(self, id):
        """Inserts an ID into the function's scope. 
        
        id: ~~ID - The ID to insert; the ID must not be qualified.
        """
        assert not id.qualified()
        self._scope[id.name()] = id 
        
    def lookupID(self, name):
        """Looks up an ID name in the function's scope.
        
        name: string - The ID name to lookup.
        
        Returns: ~~ID - The ID having the given *name*, or None if no ID with
        such a name is defined in the function's scope.
        """
        
        try:
            return self._scope[name]
        except KeyError:
            return None
    
    def IDs(self):
        """Returns all IDs defined in the function's scope.
        
        Returns: list of ~ID - List of all defined IDs.
        """
        return self._scope.values()
        
    def addBlock(self, b):
        """Adds a block to the function's implementation. Appends the ~~Block
        to the function's current list of blocks, and makes the new Block the
        he successor of the former tail Block. The new blocks own successor
        field is cleared. If the block has a name, it's added to the
        function's scope.
        
        b: ~~Block - The block to add.
        """
        if self._bodies:
            self._bodies[-1].setNext(b)
            
        self._bodies += [b] 
        b.setNext(None)
        
        if b.name():
            self.addID(id.ID(b.name(), type.Label(), id.Role.LOCAL))
    
    def blocks(self):
        """Returns all blocks making up the function's implementation.
        
        Returns: list of ~~Block - The function's current list of blocks;
        may be empty.
        """
        return self._bodies

    def clearBlocks(self):
        """Clears the function's implementation. All blocks that have been
        added to function previously, are removed. 
        """
        for b in self._bodies:
            if b.name():
                del self._scope[b.name()]
        
        self._bodies = []

    def lookupBlock(self, name):
        """Lookups the block with the given name. 
        
        name: string - The name to lookup. 
        
        Returns: ~~Block - The block with the *id's* name, or None if no such
        block exists.
        """
        for b in self._bodies:
            if b.name() == name:
                return b
        return None
    
    def location(self):
        """Returns the location associated with the function.
        
        Returns: ~~Location - The location. 
        """
        return self._location

    def __str__(self):
        return "%s" % self._name
    
    # Visitor support.
    def visit(self, visitor):
        visitor.visitPre(self)
        
        for id in self._scope.values():
            id.visit(visitor)
        
        for b in self._bodies:
            b.visit(visitor)
            
        visitor.visitPost(self)
        
    
    

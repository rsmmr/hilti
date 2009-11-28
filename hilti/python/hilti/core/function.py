# $Id$

import ast
import type
import id
import util

class Linkage:
    """The *linkage* of a ~~Function specifies its link-time
    visibility."""
    
    LOCAL = 1    
    """A ~~Function with linkage LOCAL is only visible inside the
    ~~Module it is implemented in. This is the default linkage."""

    EXPORTED = 2
    """A ~~Function with linkage EXPORTED is visible across all
    compilation units."""
    
    INIT = 3
    """A ~~Function with linkage INIT will be executed once before any other
    (non-INIT) functions defined inside the same ~~Module. The function is
    only visible inside the ~~Module it is implemented in. A function with
    linkage INIT must not receive any arguments and it must not return any
    value. Note that if an INIT function throws an exception, execution of the
    program will be terminated.
    
    Todo: Init function cannot have local variables at the moment because they
    are compiled with the ~~C linkage.
    """
        
class CallingConvention:
    """The *calling convention* used by a ~~Function specifies which
    implementation calls to it use.
    """
    
    HILTI = 1
    """A ~~Function using the proprietary HILTI calling convention.
    This convetion supports all of HILTI's flow-control features but
    cannot be called from other languages such as C. This is the
    default calling convention."""
    
    C = 2
    """A ~~Function using standard C calling convention. The
    conventions implements standard C calling semantics but does not
    support any HILTI-specific flow-control features, such as
    exceptions and continuations.
    
    Note: See :ref:`interfacing-with-C` for more information.
    
    Todo: Implementing C functions in HILTI is not fully supported at the
    moment: they can't have local variables for now because the code-generator
    would try to locate them in the HILTI stack frame, whcih doesn't exist. 
    """
    
    C_HILTI = 3
    """A ~~Function using C calling convention but receiving
    specially crafted parameters to support certain HILTI features.
    C_HILTI works mostly as the ~C convention yet with the following
    differences:

    * Arguments can have type ~~Any. 
    
    * The called function can raise an exception. 

    Note: See :ref:`interfacing-with-C` for more information.
    """

class Function(ast.Node):
    """A HILTI function declared or defined inside a module.
    
    name: string - Name of the function. It must be *non-qualified*,
    i.e., not be prefixed with any module name.
    
    ty: ~~Function - The type of the function.
    
    ident: ~~ID - The ID to which the function is bound. *id* can be initially
    None, but must be set via ~~setID before any other methods are used.
    
    location: ~~Location - A location to be associated with the function. 
    """
    
    def __init__(self, ty, ident, cc=CallingConvention.HILTI, location=None):
        assert isinstance(ty, type.Function)
        
        super(Function, self).__init__(location)
        
        self._bodies = []
        self._cc = cc
        self._type = ty
        self._id = ident
        
        # Implictly export the main function.
        self._linkage = Linkage.LOCAL
        
        self._scope = {}
        for i in ty.args():
            self._scope[i.name()] = i
            
    def name(self):
        """Returns the function's name.
        
        Returns: string - The name of the function.
        """
        return self._id.name()

    def type(self):
        """Returns the type of the function. 
        
        Returns: :class:`~hilti.core.type.Function` - The type of the
        function.
        """
        return self._type

    def ID(self): 
        """Returns the function's ID. 
        
        Returns: ~~ID - The ID to which the function is bound. 
        """
        return self._id
    
    def setID(self, id): 
        """Sets the ID to which the function is bound.
        
        id: ~~ID - The ID.
        """
        self._id = id
    
    def linkage(self):
        """Returns the linkage of the function.
        
        Returns: ~~Linkage - The linkage of the function.
        """
        
        # Always export Main::Run()
        if self._id.scope() == "main" and self._id.name() == "run":
            return Linkage.EXPORTED
        
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
        
        id: ~~ID - The ID to insert; the ID must not have a scope.
        """
        assert not id.scope()
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
        function's scope (the name must not have been used before).
        
        b: ~~Block - The block to add.
        """
        
        if b.name() and self.lookupID(b.name()):
            util.internal_error("block name %s already defined" % b.name())
        
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
                try:
                    del self._scope[b.name()]
                except:
                    pass
        
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

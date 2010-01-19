# $Id$

import binpac.support.visitor as visitor

class Node(visitor.Visitable):
    """Base class for all classes that can be nodes in an |ast|.
    """
    
    def __init__(self, location):
        self._location = location
        
    def location(self):
        """Returns the location associated with the node.
        
        Returns: ~~Location - The location. 
        """
        return self._location

    def simplify(self):
        """Simplifies the node. The definition of 'simplify' is left to the
        derived classes but the node may only change in a way thay does not
        modify its semantics.
        
        Returns: ~~Node - The simplified node, which may be just *self*.

        Note: Derived classes must not override this method but ~~doSimplify.
        """
        node = self.doSimplify()
        return node if node else self
    
    ### Method that can be overridden by derived classes.

    def validate(self, vld):
        """Validates the semantic correctness of the node.
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during validation, the
        method must call ~~Validator.error. If there are any sub-nodes that
        also need to be checked, the method needs to do that recursively.
        
        vld: ~~Validator - The validator triggering the validation.
        """
        pass
    
    def pac(self, printer):
        """Converts the statement into parseable BinPAC++ code.

        Must be overidden by derived classes.
        
        printer: ~~Printer - The printer to use.
        """
        util.internal_error("Node.pac() not overidden by %s" % self.__class__)    
        
    def resolve(self, resolver):
        """Resolves any unknown identifier the node may have. 
        
        Can be overridden by derived classes; the default implementation does
        nothing. If there are any errors encountered during resolving, the
        method must call ~~Resolver.error. If there are any sub-nodes that may 
        also have something to resolve, the method needs to do that recursively.
        
        XXXX
        
        """
        pass
    
    def doSimplify(self):
        """Simplifies the node. The definition of 'simplify' is left to the
        derived classes but the node may only change in a way thay does not
        modify its semantics.
        
        The base class does not do anything and always return *None*.
        
        Returns: ~~Node or None - If the derived classes was able to simplify
        the node, the new simpler node. If not, *None*. 
        
        Can be overidden by derived classes, in which case the superclass'
        version must be called first. If the superclass returns a simplified
        node, that one must be returned directly. 
        """
        return None
        

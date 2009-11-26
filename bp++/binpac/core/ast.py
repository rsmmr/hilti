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

    def validate(self, cg):
        """Validates the semantic correctness of this node. This
        method can be overriden by derived classes; the default
        implementation does not do anything. If there are any errors
        encountered during validation, the method must call
        ~~CodeGen.error. If there are any sub-nodes that also need
        to be checked, the method needs to do that recursively.
        
        cg: ~~CodeGen - The code generator triggering the validation.
        """
        pass
    
    def codegen(self, cg):
        """Generates HILTI code for this node. This method can be
        overriden bv derived classes; the default implementation
        does not do anything. If there are any errors encountered
        during code generation, the method must call
        ~~CodeGen.error. If there are any sub-nodes for which code
        also needs to be generated, the method needs to do that
        recursively.
        
        cg: ~~CodeGen - The code generator to use.
        """
        pass
    
        

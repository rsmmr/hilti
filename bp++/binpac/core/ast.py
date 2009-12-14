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
        

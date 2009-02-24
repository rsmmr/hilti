# $Id$

import visitor

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

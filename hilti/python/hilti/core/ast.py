# $Id$

import visitor

class Node(visitor.Visitable):
    """Base class for all classes that can be nodes in an |ast|.
    """
    
    def __init__(self, location):
        self._location = location
        self._comment = None
        
    def location(self):
        """Returns the location associated with the node.
        
        Returns: ~~Location - The location. 
        """
        return self._location
    
    def setComment(self, comment):
        """Associates a comment with this node. The comment is not further
        interpreted by the HILTI framework but will be printed out by the
        ~~Printer and can thus be used to augment a HILTI program with further
        context.
        
        comment: string or list of string - Associates either single-line
        comment with the node (if given a single string), or a multi-line
        comment (if given a list of strings). 
        
        Todo: The ~~Printer currently prints comments only for a subset of
        node types. 
        """
        if isinstance(comment, str):
            comment = [string]
            
        self._comment = comment
    
    def comment(self):
        """Returns the comment associated with this node.
        
        Returns - list of strings, or None - Each list entry represent one
        line of the comment. Returns None if no comment has been set via
        ~~setComment.
        
        Note: This function always returns a list of string. For a single-line
        comment, the list will have only a single entry.
        """
        return self._comment
        

# $Id$

import visitor

# This class is currently a no-op and used only for structuring so  that classes
# that can act as an AST node are clearly marked as such.

class Node(visitor.Visitable):
    """Node is the base class for all classes that can be nodes in an
    |ast|."""
    pass

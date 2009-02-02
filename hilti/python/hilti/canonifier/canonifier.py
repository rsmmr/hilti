# $Id$
#
# Visitor to canonify a module's code before CPS and code generation can take place.

from hilti.core import *
from hilti import instructions

### Canonify visitor.

class Canonifier(visitor.Visitor):
    """This :class:`~hilti.core.visitor.Visitor` implements the canonification
    of an |ast|, which is mandatory to perform before starting the code
    generation with :func:`~hilti.codegen.generateLLVM`. The main interface to
    the functionality provided by this package is the function
    :func:`~hilti.canonifier.canonifyAST()`. All the other package content is
    primarily for internal use by the individial handlers.
    
    Canonification proceeeds by visiting the nodes of the |ast| and building a
    new chain of transformed :class:`~hilti.core.block.Block` objects. At the
    end of the process, the old blocks are replaced with the transformed ones.
    """
    
    def __init__(self):
        super(Canonifier, self).__init__()
        self._reset()
        
    def _reset(self):
        """Resets the internal state so that the object can be used to
        canonify another |ast|.
        """
        self._transformed = None
        self._current_module = None
        self._current_function = None
        self._label_counter = 0

    def canonifyAST(self, ast):
        """Canonifies the |ast| *ast* in place."""
        self._reset()
        self.dispatch(ast)
        return True

    def currentModule(self):
        """During canonifcation, returns the current
        :class:`~hilti.core.module.Module`."""
        return self._current_module
    
    def currentFunction(self):
        """During canonifcation, returns the current
        :class:`~hilti.core.function.Function`."""
        return self._current_function

    def addTransformedBlock(self, b): 
        """Adds a :class:`~hilti.core.block.Block` *b* to the list of already
        transformed blocks."""
        self._transformed += [b]
        
    def transformedBlocks(self):
        """Returns the list of already transformed
        :class:`~hilti.core.block.Block` objects."""
        return self._transformed
        
    def currentTransformedBlock(self):
        """Returns the last block added by :meth:`transformedBlocks`, which
        can then extended with further transformed instructions."""
        try:
            return self._transformed[-1]
        except IndexError:
            return None
        
    def makeUniqueLabel(self):
        """Returns a string containing an internal label that in guarenteed to be unique
        within the function returned by :meth:`currentFunction`."""
        self._label_counter += 1
        return "@__l%d" % self._label_counter
    
canonifier = Canonifier()



    
    
    

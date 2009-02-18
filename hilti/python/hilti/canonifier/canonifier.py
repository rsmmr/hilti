# $Id$
#
# Visitor to canonify a module's code before CPS and code generation can take place.

from hilti.core import *
from hilti import instructions

### Canonify visitor.

class Canonifier(visitor.Visitor):
    """Implements the canonification of an |ast|. Canonification is mandatory
    to perform before starting the code generation with
    :func:`~hilti.codegen.generateLLVM`. 
    
    The canonification process proceeds by visiting the nodes of the |ast| and
    building a new chain of transformed :class:`~hilti.core.block.Block`
    objects. At the end of the process, the old blocks are replaced with the
    transformed ones.
    """
    
    def __init__(self):
        super(Canonifier, self).__init__()
        self._reset()
        
    def _reset(self):
        """Resets internal state. After resetting, the object can be used to
        canonify another |ast|.
        """
        self._transformed = None
        self._module = None
        self._function = None
        self._label_counter = 0

    def canonifyAST(self, ast):
        """See ~~canonifyAST."""
        self._reset()
        self.visit(ast)
        return True

    def currentModule(self):
        """Returns the current module. Only valid while canonification is in
        progress.
        
        Returns: ~~Module - The current module.
        """
        return self._module
    
    def currentFunction(self):
        """Returns the current function. Only valid while canonification is in
        progress.
        
        Returns: ~~Function - The current function.
        """
        return self._function

    def addTransformedBlock(self, b): 
        """Adds a block to the list of transformed blocks.
        
        b: ~~Block _ The block to append to the list.
        """
        self._transformed += [b]
        
    def transformedBlocks(self):
        """Returns list of transformed blocks.
        
        Returns: list of ~~Block - The list of already transformed blocks.
        """
        return self._transformed
        
    def currentTransformedBlock(self):
        """Returns the block currently being transformed. That is, the block
        which has been added :meth:`transformedBlocks`. The block can then be
        further extended.
        
        Returns: ~~Block - The block added most recently to the list of
        transformed blocks, or None if the list is still empty.
        """
        try:
            return self._transformed[-1]
        except IndexError:
            return None
        
    def makeUniqueLabel(self):
        """Generates a unique label. The label is is guarenteed to be unique
        within the function returned by :meth:`currentFunction`.
        
        Returns: string - The generated label.
        """
        self._label_counter += 1
        return "__l%d" % self._label_counter
    
canonifier = Canonifier()



    
    
    

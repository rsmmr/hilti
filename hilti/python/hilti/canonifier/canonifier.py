# $Id$
#
# Visitor to canonify a module's code before CPS and code generation can take place.

from hilti.core import *
from hilti import instructions

### Canonify visitor.

class Canonify(visitor.Visitor):
    def __init__(self):
        super(Canonify, self).__init__()
        self.reset()
        
    def reset(self):
        self._transformed = None
        self._current_module = None
        self._current_function = None
        self._label_counter = 0

    def canonifyAST(self, ast):
        self.reset()
        self.dispatch(ast)
        return True
        
canonifier = Canonify()



    
    
    

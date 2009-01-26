# $Id$
from hilti.core import *

class Checker(visitor.Visitor):
    def __init__(self):
        super(Checker, self).__init__(all=True)
        self.reset()

    def reset(self):
        self._infunction = False
        self._have_module = False
        self._errors = 0

    def checkAST(self, ast):
        self.reset()
        self.dispatch(ast)
        return self._errors
        
    def error(self, obj, str):
        self._errors += 1
        util.error(str, context=obj, fatal=False)        

checker = Checker()

        

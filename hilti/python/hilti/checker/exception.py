# $Id$
# 
# Checking exception declarations and instructions. 

from hilti.instructions import exception
from hilti.instructions import operators
from hilti.core import *
from checker import checker

@checker.when(id.ID, type.TypeDeclType)
def _(self, id):
    etype = id.type().declType()
    
    if not isinstance(etype, type.Exception):
        return

    btype = etype.baseClass()
    
    if not btype:
        self.error(id, "no base class for exception")
        return

    if btype.exceptionName() == etype.exceptionName():
        self.error(id, "exception type cannot be derived from itself")
        return

    if not etype.argType() and btype.argType() or \
       etype.argType() and btype.argType() and etype.argType() != btype.argType():
        self.error(id, "exception argument type must match that of base class (%s)" % btype.argType())
        return
        
    # Make sure there's no loop in inheritance tree.
    seen = [etype]
    while True:
        cur = seen[-1]
        parent = cur.baseClass()
        if parent.isRootType():
            break
        
        for c in seen[:-1]:
            if parent.exceptionName() == c.exceptionName():
                self.error(id, "loop in exception inheritance")
                return
            
        seen += [parent]

@checker.when(operators.New)
def _(self, i):
    if not isinstance(i.op1().value(), type.Exception):
        return

    argt = i.op1().value().argType()
    ty = i.op2().type() if i.op2() else None

    if argt and not ty:
        self.error(i, "exception argument of type %s missing" % argt)
    
    if not argt and ty:
        self.error(i, "exception type does not take an argument")
    
    if argt and ty and argt != ty:
        self.error(i, "exception argument must be of type %s" % argt)
        
        
        
    

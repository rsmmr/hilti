# $Id$
#
# Struct checks.

from hilti.instructions import struct
from hilti.core import *
from checker import checker

def _getIndex(instr):
    
    fields = instr.op1().type().refType().Fields()
    
    for i in range(len(fields)):
        (id, default) = fields[i]
        if id.name() == instr.op2().value():
            return (i, id.type())
        
    return (-1, None)

@checker.when(struct.Get)
def _(self, i):
    (idx, ftype) = _getIndex(i)
    
    if ftype != i.target().type():
        self.error(i, "target's type does not match type of struct field %s" % i.op2().value())

@checker.when(struct.Set)
def _(self, i):
    (idx, ftype) = _getIndex(i)
    
    if ftype != i.op3().type():
        self.error(i, "type of operand 3 does not match type of struct field %s" % i.op2().value())

    



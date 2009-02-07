# $Id$
#
# Overall control structures plus generic instruction printing.

from hilti.core import *
from printer import printer

@printer.when(module.Module)
def _(self, m):
    self._module = m
    self.output("module %s" % m.name())
    
### ID definitions. 

# Generic ID handler. Shouldn't be reached as we handle all cases specifically below.
@printer.when(id.ID)
def _(self, id):
    assert False, "ID type not handled: " + str(id.type().__class__)

@printer.when(id.ID, type.FunctionType)
def _(self, f):
    # Nothing to do for function bindings, there are treated separately via the 
    # Function visitor below.
    pass
    
@printer.when(id.ID, type.StructDeclType)
def _(self, id):
    assert not self._infunction

    name = id.name()
    type = id.type().type()
    
    self.output("struct %s {" % name)
    self.push()
    
    for id in type.IDs():
        self.output("%s %s, " % (id.type().name(), id.name()))
        
    self.output("}")
    self.pop()
    self.output()
    
@printer.when(id.ID, type.StorageType)
def _(self, id):
    visibility = "local" if self._infunction else "global"
    self.output("%s %s %s" % (visibility, id.type().name(), id.name()))

### Function definitions.

@printer.pre(function.Function)
def _(self, f):
    self._infunction = True
    
    type = f.type()
    
    result = type.resultType().name()
    name = f.name()
    args = ", ".join(["%s %s" % (id.type().name(), id.name())for id in type.IDs()])
    linkage = ""

    if f.linkage() == function.Linkage.EXPORTED:
        
        if not f.blocks():
            linkage = "declare "
        else:
            linkage = "extern "
        
        if f.callingConvention() == function.CallingConvention.HILTI:
            pass
            
        elif f.callingConvention() == function.CallingConvention.C:
            linkage += "\"C\" "
            
        else:
            # Cannot be reached
            assert False
    
    self.output()
    self.output("%s%s %s(%s)" % (linkage, result, name, args), nl=False)
    
    if not f.blocks():
        self.output(" {")
    else:
        self.output("")
    
    self.push()

@printer.post(function.Function)
def _(self, f):
    self._infunction = False
    
    if not f.isImported():
        self.output("}")
        
    self.pop()

### Block definitions.    
    
@printer.pre(block.Block)
def _(self, b):
    if b.name() or b.next():
        self.pop()
        self.output("")
        
        next = " # next: %s" % b.next().name() if b.next() else ""
        name = "%s: " % b.name() if b.name() else ""
        
        self.output("%s%s" % (name, next))
        self.push()

### Instructions.
    
# Generic instruction handler that can be overidden below with specialized versions.
@printer.when(instruction.Instruction)
def _(self, i):
    
    def fmtOp(op):
        if isinstance(op, instruction.IDOperand):
            return op.value().name()

        if isinstance(op, instruction.ConstOperand):
            return str(op.value())
        
        if isinstance(op, instruction.TupleOperand):
            return "(%s)" % ", ".join([fmtOp(o) for o in op.value()])
        
        return op
    
    name = i.name()
    target = "%s = " % fmtOp(i.target()) if i.target() else ""
    op1 = " %s" % fmtOp(i.op1()) if i.op1() else ""
    op2 = " %s" % fmtOp(i.op2()) if i.op2() else ""
    op3 = " %s" % fmtOp(i.op3()) if i.op3() else ""
    
    self.output("%s%s%s%s%s" % (target, name, op1, op2, op3))  

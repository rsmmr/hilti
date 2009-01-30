# $Id$
#
# Base classes for all instructions and operands provided by HIR.

builtin_type = type

import location
import type
import util

class Instruction(object):
    
    _signature = None
    
    def signature(self):
        return self.__class__._signature
    
    def __init__(self, op1=None, op2=None, op3=None, target=None, location=location.Location()):
        self._op1 = op1
        self._op2 = op2
        self._op3 = op3
        self._target = target
        self._location = location
        
        cb = self.signature().callback()
        if cb:
            cb(self)
        
    def name(self):
        return self.signature().name()
        
    def op1(self):
        return self._op1
    
    def op2(self):
        return self._op2
    
    def op3(self):
        return self._op3

    def target(self):
        return self._target

    def setOp1(self, val):
        self._op1 = val
        
    def setOp2(self, val):
        self._op2 = val
    
    def setOp3(self, val):
        self._op3 = val
        
    def setTarget(self, val):
        self._target = val
        
    def location(self):
        return self._location

    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit(self)
        
    def __str__(self):
        loc = "%s: " % self._location if self._location else ""
        target = "(%s) = " % self._target if self._target else ""
        op1 = " (%s)" % self._op1 if self._op1 else ""
        op2 = " (%s)" % self._op2 if self._op2 else ""
        op3 = " (%s)" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s%s" % (loc, target, self.signature().name(), op1, op2, op3)

# Base class. Don't instantiate.    
class Operand(object):
    def __init__(self, value, type, location):
        self._value = value
        self._type = type
        self._location = location

    def value(self):
        return self._value
    
    def type(self):
        return self._type

    def location(self):
        return self._location
    
    def __str__(self):
        return "%s %s" % (self._type, self._value)

class ConstOperand(Operand):
    def __init__(self, constant, location=location.Location()):
        super(ConstOperand, self).__init__(constant.value(), constant.type(), location)
        self._constant = constant

    def setType(self, type):
        self._constant.setType(type)
        self._type = type
        
    def constant(self):
        return self._constant

class IDOperand(Operand):
    def __init__(self, id, local, location=location.Location()):
        super(IDOperand, self).__init__(id, id.type(), location)
        self._id = id
        self._local = local

    def id(self):
        return self._id
        
    def constant(self):
        return self._constant
    
    def isLocal(self):
        return self._local
    
class TupleOperand(Operand):
    def __init__(self, ops, location=location.Location()):
        vals = ops
        types = [op.type() for op in ops]
        super(TupleOperand, self).__init__(vals, type.TupleType(types), location)
        self._ops = ops

    def __str__(self):
        return "(%s)" % ", ".join(["%s %s" % (op.type(), op.value()) for op in self._ops])
        
class Signature:
    def __init__(self, name, op1=None, op2=None, op3=None, target=None, callback=None):
        self._name = name
        self._op1 = op1
        self._op2 = op2
        self._op3 = op3
        self._target = target
        self._callback = callback
        
    def name(self):
        return self._name
    
    def op1(self):
        return self._op1
    
    def op2(self):
        return self._op2
    
    def op3(self):
        return self._op3

    def target(self):
        return self._target
    
    def callback(self):
        return self._callback

    # Turns the signature into a nicely formatted __doc__ string.
    def to_doc(self):
        
        def fmt(t, tag):
            if t:
                return " <%s>" % (type.name(t, docstring=True))
            else:
                return ""
        
        op1 = fmt(self._op1, "op1")
        op2 = fmt(self._op2, "op2")
        op3 = fmt(self._op3, "op3")
        target = fmt(self._target, "target") 
        if target:
            target += " = "
        
        doc = [".. parsed-literal::", "", "  %s**%s**%s%s%s" % (target, self._name, op1, op2, op3), ""]
    
#        if self._op1:
#            doc += ["* Operand 1: %s" % type.name(self._op1, docstring=True)]
#        
#        if self._op2:
#            doc += ["* Operand 2: %s" % type.name(self._op2, docstring=True)]
#
#        if self._op3:
#            doc += ["* Operand 3: %s" % type.name(self._op3, docstring=True)]
#
#        if target:
#            doc += ["* Target type: %s" % type.name(self._target, docstring=True)]
#
        if doc:
            doc += [""]
            
        return doc
        
    def __str__(self):
        target = "[%s] = " % self._target if self._target else ""
        op1 = " [%s]" % self._op1 if self._op1 else ""
        op2 = " [%s]" % self._op2 if self._op2 else ""
        op3 = " [%s]" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s" % (target, self._name, op1, op2, op3)


# @signature decorator (not used anymore)
def signature(name, op1=None, op2=None, op3=None, target=None, callback=None):
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, op3, target, callback)
        _Instructions[name] = ins
        return ins
    
    return register

import sys

def make_ins_init(myclass):
    def ins_init(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(self.__class__, self).__init__(op1, op2, op3, target, location)
        
    ins_init.myclass = myclass
    return ins_init
        
def instruction(name, op1=None, op2=None, op3=None, target=None, callback=None, location=None):
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, op3, target, callback)
        d = dict(ins.__dict__)
        d["__init__"] = make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
        _Instructions[name] = newclass
        return newclass
    
    return register

# Returns string representation of op; deals with tuples.
def fmt_op(op):
    if type(op) == type(()) or type(op) == type([]):
        return "(%s)" % ", ".join([str(o) for o in op])
        
    return str(op)

_Instructions = {}    

def getInstructions():
    return _Instructions

import sys

def createInstruction(name, op1=None, op2=None, op3=None, target=None, location=location.Location()):
    try:
        i = _Instructions[name](op1, op2, op3, target, location)
    except KeyError:
        return None
    
    # We assign widths to integer constants by taking the largest widths of all
    # other integer operands/target.
    #
    # FIXME: This is really not a great place to do that. We should a generic
    # capability for types to post-process arguments. 
    
    widths = [op.type().width() for op in (i.op1(), i.op2(), i.op3(), i.target()) if op and isinstance(op.type(), type.IntegerType)]
    
    maxwidth = max(widths) if widths else 0
    
    if not maxwidth:
        # Either no integer operand at all, or none with a width. As the
        # latter can only be constants, we pick a reasonable default.
        maxwidth = 32
        
    if i.op1() and isinstance(i.op1(), ConstOperand) and isinstance(i.op1().type(), type.IntegerType):
        i.op1().setType(type.IntegerType(maxwidth))
        
    if i.op2() and isinstance(i.op2(), ConstOperand) and isinstance(i.op2().type(), type.IntegerType):
        i.op2().setType(type.IntegerType(maxwidth))

    if i.op3() and isinstance(i.op3(), ConstOperand) and isinstance(i.op3().type(), type.IntegerType):
        i.op3().setType(type.IntegerType(maxwidth))

    return i

    


    
        

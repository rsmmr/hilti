# $Id$
#
# Base classes for all instructions and operands provided by HIR.

builtin_type = type

import type
import util
import location

class Instruction(object):
    
    _signature = None
    
    def signature(self):
        return self.__class__._signature
    
    def __init__(self, op1 = None, op2 = None, target = None, location = location.Location()):
        self._op1 = op1
        self._op2 = op2
        self._target = target
        self._location = location

    def name(self):
        return self.signature().name()
        
    def op1(self):
        return self._op1
    
    def op2(self):
        return self._op2

    def target(self):
        return self._target
       
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
        return "%s%s%s%s%s" % (loc, target, self.signature().name(), op1, op2)

# Base class. Don't instantiate.    
class Operand(object):
    def __init__(self, value, type):
        self._value = value
        self._type = type

    def value(self):
        return self._value
    
    def type(self):
        return self._type

    def __str__(self):
        return "%s %s" % (self._type, self._value)

class ConstOperand(Operand):
    def __init__(self, constant):
        super(ConstOperand, self).__init__(constant.value(), constant.type())
        self._constant = constant

    def constant(self):
        return self._constant

class IDOperand(Operand):
    def __init__(self, id):
        super(IDOperand, self).__init__(id.name(), id.type())
        self._id = id

    def constant(self):
        return self._constant
    
class TupleOperand(Operand):
    def __init__(self, ops):
        vals = [op.value() for op in ops]
        types = [op.type() for op in ops]
        super(TupleOperand, self).__init__(vals, type.TupleType(types))
        self._ops = ops

    def __str__(self):
        return "(%s)" % ", ".join(["%s %s" % (op.type(), op.value()) for op in self._ops])
        
class Signature:
    def __init__(self, name, op1 = None, op2 = None, target = None):
        self._name = name
        self._op1 = op1
        self._op2 = op2
        self._target = target
        
    def name(self):
        return self._name
    
    def op1(self):
        return self._op1
    
    def op2(self):
        return self._op2

    def target(self):
        return self._target

    # Turns the signature into a nicely formatted __doc__ string.
    def to_doc(self):
        op1 = " *op1*" if self._op1 else "" 
        op2 = " *op2*" if self._op2 else "" 
        target = "*target* = " if self._target else "" 
        
        doc = [".. parsed-literal::", "", "  %s**%s**%s%s" % (target, self._name, op1, op2), ""]
    
        if self._op1:
            doc += ["* Operand 1: %s" % type.name(self._op1, docstring=True)]
        
        if self._op2:
            doc += ["* Operand 2: %s" % type.name(self._op1, docstring=True)]

        if target:
            doc += ["* Target type: %s" % type.name(self._target, docstring=True)]

        if doc:
            doc += [""]
            
        return doc
        
    def __str__(self):
        target = "[%s] = " % self._target if self._target else ""
        op1 = " [%s]" % self._op1 if self._op1 else ""
        op2 = " [%s]" % self._op2 if self._op2 else ""
        return "%s%s%s%s" % (target, self._name, op1, op2)


# @signature decorator (not used anymore)
def signature(name, op1 = None, op2 = None, target = None):
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, target)
        _Instructions[name] = ins
        return ins
    
    return register

import sys

def make_ins_init(myclass):
    def ins_init(self, name, op1 = None, op2 = None, target = None):
        super(self.__class__, self).__init__(name, op1, op2, target)
        
    ins_init.myclass = myclass
    return ins_init
        
def instruction(name, op1 = None, op2 = None, target = None):
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, target)
        _Instructions[name] = ins
        d = dict(ins.__dict__)
        d["__init__"] = make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
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
        
    
        

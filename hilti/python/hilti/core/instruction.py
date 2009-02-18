# $Id$

builtin_type = type

import ast
import location
import type
import util
import constant

class Instruction(ast.Node):
    """This class is the base class for all instructions supported by the
    HILTI language. It is however not supposed to be derived directly from; to
    create a new instruction, use the :meth:`instruction` decorator.
    
    *op1*/*op2*/*op3* are the ~~Operand objects of the instruction. *target*
    specifies where the instructions result is stored and must be an
    ~~IDOperand. The types of operands and targt must match with the
    Instruction's ~~Signature and may only be *None* if the signature allows
    for it (including for optional operands). *location* gives a ~~Location to
    be associated with this Function.
    """
    
    _signature = None
    
    def __init__(self, op1=None, op2=None, op3=None, target=None, location=None):
        assert not target or isinstance(target, IDOperand)
        
        self._op1 = op1
        self._op2 = op2
        self._op3 = op3
        self._target = target
        self._location = location
        
        cb = self.signature().callback()
        if cb:
            cb(self)

    def signature(self):
        """Returns the ~~Signature of this Instruction."""
        return self.__class__._signature
            
    def name(self):
        """Returns a string with the Instruction's name, i.e., the mnemonic as
        used in a HILTI program."""
        return self.signature().name()
        
    def op1(self):
        """Returns the Instruction's first ~~Operand, or *None* if it does not
        have any operands."""
        return self._op1
    
    def op2(self):
        """Returns the Instruction's second ~~Operand, or *None* if it has
        less than two operands."""
        return self._op2
    
    def op3(self):
        """Returns the Instruction's second ~~Operand, or *None* if it has
        less than three operands."""
        return self._op3

    def target(self):
        """Returns the Instruction's target ~~IDOperand, or *None* if it does
        not have target."""
        return self._target

    def setOp1(self, op):
        """Sets the ~~Operand *op* to be the Instruction's first operand. The
        type of *op* must match with the Instruction's ~~Signature."""
        self._op1 = op
        
    def setOp2(self, op):
        """Sets the ~~Operand *op* to be the Instruction's second operand. The
        type of *op* must match with the Instruction's ~~Signature."""
        self._op2 = op
    
    def setOp3(self, op):
        """Sets the ~~Operand *op* to be the Instruction's third operand. The
        type of *op* must match with the Instruction's ~~Signature."""
        self._op3 = op
        
    def setTarget(self, target):
        """Sets the ~~IDOperand *target* to be the Instruction's target for
        results. The type of *target* must match the Instruction's
        ~~Signature."""
        assert not target or isinstance(target, IDOperand)
        self._target = target
        
    def location(self):
        """Returns the ~~Location object associated with this Instruction"""
        return self._location

    def __str__(self):
        target = "(%s) = " % self._target if self._target else ""
        op1 = " (%s)" % self._op1 if self._op1 else ""
        op2 = " (%s)" % self._op2 if self._op2 else ""
        op3 = " (%s)" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s" % (target, self.signature().name(), op1, op2, op3)

class Operand(object):
    """This is the base class for operands and targets of ~~Instruction
    objects. It is not intended for direct use; instantiate one of its derived
    classes instead.
    
    *value* is the Operand's value, with a type determined by the derived
    class. *type* is the Operand ~~Type, and *location* a ~~Location to be
    associated with the operand.
    
    (Note that this class does *not* implement ~~Visitor support as an Operand
    is not itself an ~~Node.)
    """
    def __init__(self, value, type, location):
        self._value = value
        self._type = type
        self._location = location

    def value(self):
        """Returns the value of the Operand. The value's type depends on the
        derived class"""
        return self._value
    
    def type(self):
        """Returns the Operand's ~~Type."""
        return self._type

    def location(self):
        """Returns the ~~Location object associated with this Instruction"""
        return self._location

    def _setValue(self, value):
        self._value = value
        
    def _setType(self, type):
        self._type = type
    
    def __str__(self):
        return "%s" % self._value

class ConstOperand(Operand):
    """Derived from ~~Operand, a ConstOperand represents a constant
    ~~Instruction operand. *constant* is an instance of ~~Constant, and
    *location* a ~~Location to be associated with the operand.
    
    For a ConstOperand, :meth:`value()` returns the same value as Constant's
    ~~Constant.value.
    """
    def __init__(self, constant, location=None):
        super(ConstOperand, self).__init__(constant.value(), constant.type(), location)
        self._constant = constant

    def constant(self):
        """Returns the operand's ~~Constant"""
        return self._constant
    
    def setConstant(self, const):
        """Sets the operand's ~~Constant to *const*"""
        self._constant = const
        self._setValue(const.value())
        self._setType(const.type())
        
class IDOperand(Operand):
    """Derived from ~~Operand, an IDOperand is an operand that references an
    ~~ID. *id* is the reference ~~ID, , and *location* a ~~Location to be
    associated with the operand.
    
    For an IDOperand, :meth:`value()` returns the name of the ~~ID as a
    string.
    """
    
    def __init__(self, id, location=None):
        super(IDOperand, self).__init__(id, id.type(), location)
        self._id = id

    def id(self):
        """Returns the operand's ~~ID."""
        return self._id
    
    def setID(self, id):
        """Sets the ID.
        
        id: ~~ID - The ID to set for the operand."""
        self._id = id
        self._setValue(id)
        self._setType(id.type())

        
class TupleOperand(Operand):
    """Derived from ~~Operand, a TupleOperand is an operand consisting of a
    tupel of individual operands. *ops* is a list of the tuple's ~~Operand
    objects, and *location* a ~~Location to be associated with the operand.
    
    For a TupleOperand, :meth:`value()` returns the list of ~~Operand`
    objects, and :meth:`value() returns a list of the individual operands'
    ~~Type.
    """
    def __init__(self, ops, location=None):
        vals = ops
        types = [op.type() for op in ops]
        super(TupleOperand, self).__init__(vals, type.Tuple(types), location)
        self._ops = ops
    
    def __str__(self):
        return "(%s)" % ", ".join([str(op) for op in self._ops])
        
class Signature:
    """A Signature defines an ~~Instruction 's name as well as the valid
    ~~Type for operands and target. The class is not supposed to be
    instantiated directly; instances are implicitly created when using the
    :func:`instruction` decorator.
    
    *name* is a string giving the instruction's HILTI mnemonic. 
    *op1*/*op2*/*op3* give the ~~Type of the instructions operands, and
    *target* is the ~~Type of the instruction's result; these types must be
    the corresponding *classes*, not instances thereof. If any of these is
    *None*, it indicates that the instruction does not use the corresponding
    operand or returns any result, respectively. If one of the types is a
    tuple of types, that indicates that any of tuple's members is a valid type
    for the operand/result. If *None* is a member of the tuple, that indicates
    that the operand/result is optional. *callback* is function that is
    called each time an ~~Instruction has been instantiated with this
    Signature; the callback gets the ~~Instruction as its only parameter.
    *terminator* is a boolean indicating whether the Signature define's an
    instruction that is considered a ~~Block |terminator|.
    """
    def __init__(self, name, op1=None, op2=None, op3=None, target=None, callback=None, terminator=False):
        self._name = name
        self._op1 = op1
        self._op2 = op2
        self._op3 = op3
        self._target = target
        self._callback = callback
        self._terminator = terminator
        
    def name(self):
        """Returns the name (mnemonic) which the Signature for an
        ~~Instruction."""
        return self._name
    
    def op1(self):
        """Returns the ~~Type of the first ~~Operand"""
        return self._op1
    
    def op2(self):
        """Returns the ~~Type of the second ~~Operand"""
        return self._op2
    
    def op3(self):
        """Returns the ~~Type of the third ~~Operand"""
        return self._op3

    def target(self):
        """Returns the ~~Type of the result."""
        return self._target
    
    def callback(self):
        """Returns the Signature's callback function."""
        return self._callback
    
    def terminator(self):
        """Returns true if the Signature defines a |terminator| instruction."""
        return self._terminator
        
    def __str__(self):
        target = "[%s] = " % self._target if self._target else ""
        op1 = " [%s]" % self._op1 if self._op1 else ""
        op2 = " [%s]" % self._op2 if self._op2 else ""
        op3 = " [%s]" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s" % (target, self._name, op1, op2, op3)

def _make_ins_init(myclass):
    def ins_init(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(self.__class__, self).__init__(op1, op2, op3, target, location)
        
    ins_init.myclass = myclass
    return ins_init
        
def instruction(name, op1=None, op2=None, op3=None, target=None, callback=None, terminator=False, location=None):
    """This function is a *decorater* for classes derived from ~~Instruction;
    it defines the new instruction's ~~Signature. The arguments are the same
    as taken by ~~Signature constructor."""
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, op3, target, callback, terminator)
        d = dict(ins.__dict__)
        d["__init__"] = _make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
        _Instructions[name] = newclass
        return newclass
    
    return register

_Instructions = {}    

def getInstructions():
    """Returns a list of all classes derived from ~~Instruction, i.e., a
    complete list of all instructions provided by the HILTI language."""
    return _Instructions

def createInstruction(name, op1=None, op2=None, op3=None, target=None, location=None):
    """Instantiates a new instruction according to the instruction *name*. The
    name must correspond to the mnemonic as defined by :meth:`instruction`
    decorated subclass of ~~Instruction. *createInstruction* instantiates a
    object of this class and initializes *op1*/*op2*/*op3*, *target* and
    *location* accordingly (see ~~Instruction)."""
    
    try:
        i = _Instructions[name](op1, op2, op3, target, location)
    except KeyError:
        return None

    return i

    


    
        

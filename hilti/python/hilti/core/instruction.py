# $Id$

builtin_type = type

import location
import type
import util
import constant

class Instruction(object):
    """This class is the base class for all instructions supported by the
    HILTI language. It is however not supposed to be derived directly from; to
    create a new instruction, use the :meth:`instruction` decorator.
    
    *op1*/*op2*/*op3* are the :class:`Operand` objects of the instruction.
    *target* specifies where the instructions result is stored and must be an
    :class:`IDOperand`. The types of operands and targt must match with the
    Instruction's :class:`Signature` and may only be *None* if the signature
    allows for it (including for optional operands). *location* gives a
    :class:`~hilti.core.location.Location` to be associated with this
    Function.
    
    This class implements :class:`~hilti.core.visitor.Visitor` support without
    subtypes. 
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
        """Returns the :class:`Signature` of this Instruction."""
        return self.__class__._signature
            
    def name(self):
        """Returns a string with the Instruction's name, i.e., the mnemonic as
        used in a HILTI program."""
        return self.signature().name()
        
    def op1(self):
        """Returns the Instruction's first :class:`Operand`, or *None* if it
        does not have any operands."""
        return self._op1
    
    def op2(self):
        """Returns the Instruction's second :class:`Operand`, or *None* if it
        has less than two operands."""
        return self._op2
    
    def op3(self):
        """Returns the Instruction's second :class:`Operand`, or *None* if it
        has less than three operands."""
        return self._op3

    def target(self):
        """Returns the Instruction's target :class:`IDOperand`, or *None* if
        it does not have target."""
        return self._target

    def setOp1(self, op):
        """Sets the :class:`Operand` *op* to be the Instruction's first
        operand. The type of *op* must match with the Instruction's
        :class:`Signature`."""
        self._op1 = op
        
    def setOp2(self, op):
        """Sets the :class:`Operand` *op* to be the Instruction's second
        operand. The type of *op* must match with the Instruction's
        :class:`Signature`."""
        self._op2 = op
    
    def setOp3(self, op):
        """Sets the :class:`Operand` *op* to be the Instruction's third
        operand. The type of *op* must match with the Instruction's
        :class:`Signature`."""
        self._op3 = op
        
    def setTarget(self, target):
        """Sets the :class:`IDOperand` *target* to be the Instruction's target
        for results. The type of *target* must match the Instruction's
        :class:`Signature`."""
        assert not target or isinstance(target, IDOperand)
        self._target = target
        
    def location(self):
        """Returns the :class:`~hilti.core.location.Location` object
        associated with this Instruction"""
        return self._location

    # Visitor support.
    def dispatch(self, visitor):
        visitor.visit(self)
        
    def __str__(self):
        target = "(%s) = " % self._target if self._target else ""
        op1 = " (%s)" % self._op1 if self._op1 else ""
        op2 = " (%s)" % self._op2 if self._op2 else ""
        op3 = " (%s)" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s" % (target, self.signature().name(), op1, op2, op3)

class Operand(object):
    """This is the base class for operands and targets of :class:`Instruction`
    objects. It is not intended for direct use; instantiate one of its derived
    classes instead.
    
    *value* is the Operand's value, with a type determined by the derived
    class. *type* is the Operand :class:`~hilti.type.Type`, and *location* a
    :class:`~hilti.core.location.Location` to be associated with the operand.
    
    (Note that this class does *not* implement
    :class:`~hilti.core.visitor.Visitor` support.)
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
        """Returns the Operand's :class:`~hilti.type.Type`."""
        return self._type

    def location(self):
        """Returns the :class:`~hilti.core.location.Location` object
        associated with this Instruction"""
        return self._location

    def _setValue(self, value):
        self._value = value
        
    def _setType(self, type):
        self._type = type
    
    def __str__(self):
        return "%s %s" % (self._type, self._value)

class ConstOperand(Operand):
    """Derived from :class:`Operand`, a ConstOperand represents a constant
    :class:`Instruction` operand. *constant* is an instance of
    :class:`~hilti.core.constant.Constant`, and *location* a
    :class:`~hilti.core.location.Location` to be associated with the operand.
    
    For a ConstOperand, :meth:`value()` returns the same value the
    :class:`~hilti.core.constant.Constant`
    :meth:`~hilti.core.constant.Constant.value` method. 
    """
    def __init__(self, constant, location=location.Location()):
        super(ConstOperand, self).__init__(constant.value(), constant.type(), location)
        self._constant = constant

    def constant(self):
        """Returns the operand's :class:`~hilti.core.constant.Constant`"""
        return self._constant
    
    def setConstant(self, const):
        """Sets the operand's :class:`~hilti.core.constant.Constant` to *const*"""
        self._constant = const
        self._setValue(const.value())
        self._setType(const.type())

class IDOperand(Operand):
    """Derived from :class:`Operand`, an IDOperand is an operand
    that references an :class:`~hilti.core.id.ID`. *id* is the
    reference :class:`~hilti.core.id.ID`, , and *location* a
    :class:`~hilti.core.location.Location` to be associated with the
    operand.
    
    For an IDOperand, :meth:`value()` returns the name of the 
    :class:`~hilti.core.id.ID` as a string.
    """
    
    def __init__(self, id, location=location.Location()):
        super(IDOperand, self).__init__(id, id.type(), location)
        self._id = id

    def id(self):
        """Returns the operand's :class:`~hilti.core.id.ID`."""
        return self._id
    
class TupleOperand(Operand):
    """Derived from :class:`Operand`, a TupleOperand is an operand
    consisting of a tupel of individual operands. *ops* is a list of
    the tuple's :class:`Operand` objects, and *location* a
    :class:`~hilti.core.location.Location` to be associated with the
    operand.
    
    For a TupleOperand, :meth:`value()` returns the list of
    :class:`Operand` objects, and :meth:`value()` returns a list of
    the individual operands' :class:`~hilti.core.type.Type`.
    """
    def __init__(self, ops, location=location.Location()):
        vals = ops
        types = [op.type() for op in ops]
        super(TupleOperand, self).__init__(vals, type.TupleType(types), location)
        self._ops = ops

    def __str__(self):
        return "(%s)" % ", ".join(["%s %s" % (op.type(), op.value()) for op in self._ops])
        
class Signature:
    """A Signature defines an :class:`~hilti.Instruction` 's name as well as
    the valid :class:`~hilti.core.type.Type` for operands and target. The
    class is not supposed to be instantiated directly; instances are
    implicitly created when using the :meth:`instruction` decorator.
    
    *name* is a string with the instruction's HILTI mnemonic. 
    *op1*/*op2*/*op3* are types of instructions operands, and *target* is the
    type of the instruction's result. If any of these is *None*, that
    indicates that the instruction does not use the corresponding operand or
    return any result, respectively. *callback* is function that is called
    each time an :class:`Instruction` has been instantiated with this
    Signature; the callback gets the :class:`Instruction` as its only
    parameter. *terminator* is a boolean indicating whether the Signature
    define's an instruction that is considered a
    :class:`~hilti.core.block.Block` |terminator|.
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
    
    def terminator(self):
        return self._terminator

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


def make_ins_init(myclass):
    def ins_init(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(self.__class__, self).__init__(op1, op2, op3, target, location)
        
    ins_init.myclass = myclass
    return ins_init
        
def instruction(name, op1=None, op2=None, op3=None, target=None, callback=None, terminator=False, location=None):
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, op3, target, callback, terminator)
        d = dict(ins.__dict__)
        d["__init__"] = make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
        _Instructions[name] = newclass
        return newclass
    
    return register

_Instructions = {}    

def getInstructions():
    return _Instructions

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

    def adaptIntType(op):
        if op and isinstance(op, ConstOperand) and isinstance(op.type(), type.IntegerType):
            val = op.value()
            const = constant.Constant(val, type.IntegerType(maxwidth))
            op.setConstant(ConstOperand(const))
        
    adaptIntType(i.op1())
    adaptIntType(i.op2())
    adaptIntType(i.op3())

    return i

    


    
        

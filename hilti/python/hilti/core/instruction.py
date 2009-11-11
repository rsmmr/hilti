# $Id$

builtin_type = type

import ast
import type
import util
import constant

class Instruction(ast.Node):
    """Base class for all instructions supported by the HILTI language.
    To create a new instruction do however *not* derive directly from
    Instruction but use the :meth:`instruction` decorator.
    
    op1: ~~Operand - The instruction's first operand, or None if unused.
    op2: ~~Operand - The instruction's second operand, or None if unused.
    op3: ~~Operand - The instruction's third operand, or None if unused.
    target: ~~IDOperand - The instruction's target, or None if unused.
    location: ~~Location - A location to be associated with the instruction. 
    """
    
    _signature = None
    
    def __init__(self, op1=None, op2=None, op3=None, target=None, location=None):
        assert not target or isinstance(target, IDOperand)
        
        super(Instruction, self).__init__(location)
        self._op1 = op1
        self._op2 = op2
        self._op3 = op3
        self._target = target
        self._location = location
        
        cb = self.signature().callback()
        if cb:
            cb(self)

    def signature(self):
        """Returns the instruction's signature.
        
        Returns: ~~Signature - The signature.
        """
        return self.__class__._signature
            
    def name(self):
        """Returns the instruction's name. The name is the mnemonic as used in
        a HILTI program.
        
        Returns: string - The name.
        """
        return self.signature().name()
        
    def op1(self):
        """Returns the instruction's first operand.
        
        Returns: ~~Operand - The operand, or None if unused.
        """
        return self._op1
    
    def op2(self):
        """Returns the instruction's second operand.
        
        Returns: ~~Operand - The operand, or None if unused.
        """
        return self._op2
    
    def op3(self):
        """Returns the instruction's third operand.
        
        Returns: ~~Operand - The operand, or None if unused.
        """
        return self._op3

    def target(self):
        """Returns the instruction's target.
        
        Returns: ~~IDOperand - The target, or None if unused.
        """
        return self._target

    def setOp1(self, op):
        """Sets the instruction's first operand.
        
        op: ~~Operand - The new operand to set.
        """
        self._op1 = op
        
    def setOp2(self, op):
        """Sets the instruction's second operand.
        
        op: ~~Operand - The new operand to set.
        """
        self._op2 = op
    
    def setOp3(self, op):
        """Sets the instruction's third operand.
        
        op: ~~Operand - The new operand to set.
        """
        self._op3 = op
        
    def setTarget(self, target):
        """Sets the instruction's target.
        
        op: ~~IDOperand - The new operand to set.
        """
        assert not target or isinstance(target, IDOperand)
        self._target = target
        
    def __str__(self):
        target = "(%s) = " % self._target if self._target else ""
        op1 = " (%s)" % self._op1 if self._op1 else ""
        op2 = " (%s)" % self._op2 if self._op2 else ""
        op3 = " (%s)" % self._op3 if self._op3 else ""
        return "%s%s%s%s%s" % (target, self.signature().name(), op1, op2, op3)

    # Visitor support.
    def visit(self, visitor):
        visitor.visitPre(self)

        if self._op1:
            self._op1.visit(visitor)
            
        if self._op2:
            self._op2.visit(visitor)
            
        if self._op3:
            self._op3.visit(visitor)

        if self._target:
            self._target.visit(visitor)

        visitor.visitPost(self)

class Operator(Instruction):
    """Class for instructions that are overloaded by type. While most
    HILTI instructions are clearly tied to a particular type, *operators* are
    generic instructions that can operator on different types.
    
    To create a new operator do *not* derive directly from Operator but use
    the :meth:`operator` decorator.
    
    op1: ~~Operand - The operator's first operand, or None if unused.
    op2: ~~Operand - The operator's second operand, or None if unused.
    op3: ~~Operand - The operator's third operand, or None if unused.
    target: ~~IDOperand - The operator's target, or None if unused.
    location: ~~Location - A location to be associated with the operator. 
    """
    def __init__(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(Operator, self).__init__(op1, op2, op3, target, location)

class OverloadedOperator(Instruction):
    """Class for overloading an Operator on a type-specific basis. For each
    ~~Operator a type wants to provide an overloaded implementation, it must
    be define an OverloadedOperator. However, to do so do *not* derive
    directly from this class but use the :meth:`overload` decorator.

    operator: ~~Operator class - The operator to be overloaded. 
    op1: ~~Operand - The operator's first operand.
    op2: ~~Operand - The operator's second operand, or None if unused.
    op3: ~~Operand - The operator's third operand, or None if unused.
    target: ~~IDOperand - The operator's target, or None if unused.
    location: ~~Location - A location to be associated with the operator. 
    """
        
    def __init__(self, operator, op1, op2=None, op3=None, target=None, location=None):
        super(Operator, self).__init__(op1, op2, op3, target, location)
        self._operator = operator
        
    def operator():
        """Returns the operator which is overloaded.
        
        Returns: ~~Operator class - The overload operator.
        """
        return self._operator
    
class Operand(ast.Node):
    """Base class for operands and targets of HILTI instructions. 
    
    value: any - The operand's value; must match with the *type*.
    type: ~~Type - The operand's type.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, value, type, location):
        super(Operand, self).__init__(location)
        self._value = value
        self._type = type
        self._location = location
        
        self._refined = False # For internal use in signature.matchWithInstruction()

    def value(self):
        """Return's the operand's value. 
        
        Returns: object - The operand's value.
        """
        return self._value
    
    def type(self):
        """Return's the operand's type. 
        
        Returns: object - The operand's type.
        """
        return self._type

    def _setValue(self, value):
        self._value = value
        
    def _setType(self, type):
        self._type = type
    
    def __str__(self):
        return str(self._value)

class ConstOperand(Operand):
    """Represents a constant operand. For a ConstOperand, :meth:`value()`
    returns the same value as the corresponding constant's ~~Constant.value.
    
    constant: ~~Constant - The constant value.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, constant, location=None):
        super(ConstOperand, self).__init__(constant.value(), constant.type(), location)
        self._constant = constant

    def constant(self):
        """Returns the operand's constant.
        
        Returns: ~~Constant - The constant value."""
        return self._constant
    
    def setConstant(self, const):
        """Sets the operand's constant.
        
        const: ~~Constant - The new constant value.
        """
        self._constant = const
        self._setValue(const.value())
        self._setType(const.type())
        
class IDOperand(Operand):
    """Represents an ID operand. For an IDOperand, :meth:`value()`
    returns the name of the corresponding ~~ID.
    
    id: ~~ID - The operand's ID.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, id, location=None):
        super(IDOperand, self).__init__(id, id.type(), location)
        self._id = id

    def id(self):
        """Returns the operand's ID.
        
        Returns: ~~ID - The operand's ID.
        """       
        return self._id
    
    def setID(self, id):
        """Sets the operand's ID.
        
        id: ~~ID - The operand's new ID.
        """
        self._id = id
        self._setValue(id)
        self._setType(id.type())

class TupleOperand(Operand):
    """Represents a tuple operand. For a TupleOperand, :meth:`value()` returns
    a list of the individual ~~Operand objects, and :meth:`type() returns a
    list of the individual operands' ~~Type objects.
    
    ops: list of ~~Operand - The individual operands of the tuple.
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, ops, location=None):
        vals = ops
        types = [op.type() for op in ops]
        super(TupleOperand, self).__init__(vals, type.Tuple(types), location)
        self._ops = ops
    
    def setTuple(self, ops):
        """Set the operand's tuple.
        
        ops: tuple of ~~Operand - The new tuple of operands.
        """
        types = [op.type() for op in ops]
        self._setValue(ops)
        self._setType(type.Tuple(types))
        self._ops = ops
        
    def __str__(self):
        return "(%s)" % ", ".join([str(op) for op in self._ops])

class TypeOperand(Operand):
    """Represents a type as an operand. For a TypeOperand, :meth:`value()`
    returns the ~~Type the operand refers to, and :meth:`type() returns 
    a corresponding ~~TypeDeclType.
     
    t: ~~Type - The type the operand refers to. 
    location: ~~Location - A location to be associated with the operand. 
    """
    def __init__(self, t, location=None):
        super(TypeOperand, self).__init__(t, type.TypeDeclType(t), location)
    
    def __str__(self):
        return str(self.value())

from signature import *
    
def _make_ins_init(myclass):
    def ins_init(self, op1=None, op2=None, op3=None, target=None, location=None):
        super(self.__class__, self).__init__(op1, op2, op3, target, location)
        
    ins_init.myclass = myclass
    return ins_init
        
def instruction(name, op1=None, op2=None, op3=None, target=None, callback=None, terminator=False, location=None):
    """A *decorater* for classes derived from ~~Instruction. The decorator
    defines the new instruction's ~~Signature. The arguments correpond to
    those of the ~~Signature constructor."""
    def register(ins):
        global _Instructions
        ins._signature = Signature(name, op1, op2, op3, target, callback, terminator)
        d = dict(ins.__dict__)
        d["__init__"] = _make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
        _Instructions[name] = newclass
        return newclass
    
    return register

# Currently, "operator" is just an alias for "instruction".
operator = instruction
"""A *decorater* for classes derived from ~~Operator. The decorator
defines the new operators's ~~Signature. The arguments correpond to
those of the ~~Signature constructor."""

def overload(operator, op1, op2=None, op3=None, target=None):
    """A *decorater* for classes that provide type-specific overloading of an
    operator. The decorator defines the overloaded operator's ~~Signature. The
    arguments correpond to those of the ~~Signature constructor except for
    *operator* which must be a subclass of ~~Operator."""
    def register(ins):
        global _OverloadedOperators
        assert issubclass(operator, Operator)

        global _Instructions
        ins._signature = Signature(operator().name(), op1, op2, op3, target)
        d = dict(ins.__dict__)
        d["__init__"] = _make_ins_init(ins)
        newclass = builtin_type(ins.__name__, (ins.__base__,), d)
        
        idx = operator.__name__
        try:
            _OverloadedOperators[idx] += [ins]
        except:
            _OverloadedOperators[idx] = [ins]
            
        return newclass
    
    return register

_Instructions = {}    
_OverloadedOperators = {}

def getInstructions():
    """Returns a dictionary of instructions. More precisely, the function
    returns all classes decorated with either ~~Instruction or ~~operator;
    these classes will be all be derived from ~~Instruction and represent a
    complete enumeration of all instructions provided by the HILTI language.
    The dictionary is indexed by the instruction name and maps the name to the
    ~~Instruction instance.
    
    Returns: dictionary of ~~Instruction-derived classes - The list of all
    instructions.
    """
    return _Instructions

def createInstruction(name, op1=None, op2=None, op3=None, target=None, location=None):
    """Instantiates a new instruction. 
    
    name: The name of the instruction to be instantiated; i.e., the mnemnonic
    as defined by a :meth:`instruction` decorator.
    
    op1: ~~Operand - The instruction's first operand, or None if unused.
    op2: ~~Operand - The instruction's second operand, or None if unused.
    op3: ~~Operand - The instruction's third operand, or None if unused.
    target: ~~IDOperand - The instruction's target, or None if unused.
    location: ~~Location - A location to be associated with the instruction. 
    """
    try:
        i = _Instructions[name](op1, op2, op3, target, location)
    except KeyError:
        return None

    return i

def findOverloadedOperator(op):
    """Returns the type-specific version(s) of an overloaded operator. Based on
    an instance of ~~Operator, it will search all type-specific
    implementations (i.e., all ~~OverloadedOperators) and return the matching
    ones. 
    
    op: ~~Operator - The operator for which to return the type-specific version.
    
    Returns: list of ~~OverloadedOperator - The list of matching operater
    implementations.
    """
    
    matches = []
    
    try:
        for o in _OverloadedOperators[op.__class__.__name__]:
            (success, errormsg) = o._signature.matchWithInstruction(op)
            if success:
                matches += [o]
    except KeyError:
        pass

    return matches

# $Id$
"""The ``constraints`` module predefines the following constraint
and refinement functions. See :ref:`signature-constraints` for more
information about such functions. Note that some of these functions
are actually not constraint function themselves but *return*
constraint functions when called. 
""" 

from instruction import *
from signature import *

Constant = constant.Constant # We override constant() later. 

# Returns operator number *n* in instruction *i*, where n=0...3, with 0 being
# the target.
def _getOp(i, n):
    assert n >= 0 and n <= 3
    
    if n == 0:
        return i.target()
        
    if n == 1:
        return i.op1()
            
    if n == 2:
        return i.op2()
            
    return i.op3()

# Returns operator number *n* in signature *s*, where n=0...3, with 0 being
# the target.
def _getSigOp(s, n):
    assert n >= 0 and n <= 3
    
    if n == 0:
        return s.target()
        
    if n == 1:
        return s.op1()
            
    if n == 2:
        return s.op2()
            
    return s.op3()

# Returns a name to refer to operand *n*, where n=0...3, with 0 being the
# target.
def _getOpName(n):
    assert n >= 0 and n <= 3
    
    if n == 0:
        return "target"
    
    return "operand %d" % n

def optional(constr):
    """Returns a constraint function that checks whether an operand is either
    not in place or, if given, conforms with another *constraint* function."""
    assert_constraint(constr)
    @constraint(lambda sig: "[%s]" % sig.getOpDoc(constr))
    def _optional(ty, op, i):
        return constr(ty, op, i) if op else (True, "")

    # This is a hack to mark the optional() costraint.
    _optional._optional = True
    return _optional
    
def constant(constr):
    """Returns a constraint function that checks whether an operand is a
    constant conforming with the additional constraint function *constraint*."""
    assert_constraint(constr)
    @constraint(lambda sig: sig.getOpDoc(constr))
    def _isConstant(ty, op, i):
        return constr(ty, op, i) if (op and isinstance(op, ConstOperand)) else (False, "must be a constant")
        
    return _isConstant

def isType(constr):
    """Returns a constraint function that checks whether an operand is a
    ~~TypeOperand conforming with *constraint*."""
    assert_constraint(constr)
    @constraint(lambda sig: "%s" % sig.getOpDoc(constr))
    def _isType(ty, op, i):
        if not op or not isinstance(op, TypeOperand):
            return (False, "must be a type")
        return constr(op.value(), None, i)
        
    return _isType

def refineIntegerConstant(w):
    """Returns a type refinement function that sets a constant integer
    operand's type to be of width *n* if its value is within the valid
    range."""
    def _refineIntegerConstant(op, i):
        if not isinstance(op, ConstOperand) or not isinstance(op.type(), type.Integer):
            return
        
        if abs(op.value()) < 2**(w-1):
            # Const is in the right range, reset the type.
            op.setConstant(Constant(op.value(), type.Integer(w)))
            
    return _refineIntegerConstant

def refineIntegerConstantWithOp(n):
    """Returns a type refinement function that sets a constant integer to the
    same width as operand *n* if its value is within the valid range."""
    def _refineIntegerConstant(op, i):
        if not isinstance(op, ConstOperand) or not isinstance(op.type(), type.Integer):
            return
        
        w = _getOp(i, n).type().width()
        
        if abs(op.value()) < 2**(w-1):
            # Const is in the right range, reset the type.
            op.setConstant(Constant(op.value(), type.Integer(w)))
            
    return _refineIntegerConstant

def refineIntegerConstantWithOpItemType(n):
    """Returns a type refinement function that sets a constant integer to the
    same width as operand *n*'s item-type; the operand must be a reference to
    a container."""
    def _refineIntegerConstant(op, i):
        if not isinstance(op, ConstOperand) or not isinstance(op.type(), type.Integer):
            return
        
        t = _getOp(i, n).type().refType()
        if not isinstance(t, type.Container):
            return

        t = t.itemType()
        
        if not isinstance(t, type.Integer):
            return
   
        w = t.width()
        
        if abs(op.value()) < 2**(w-1):
            # Const is in the right range, reset the type.
            op.setConstant(Constant(op.value(), type.Integer(w)))
            
    return _refineIntegerConstant

def refineIntegerConstantWithOpDerefType(n):
    """Returns a type refinement function that sets a constant integer to the
    same width as operand *n*'s dereference; the operand must be an interator.
    """
    def _refineIntegerConstant(op, i):
        if not isinstance(op, ConstOperand) or not isinstance(op.type(), type.Integer):
            return
        
        t = _getOp(i, n).type().containerType().itemType()
        
        if not isinstance(t, type.Integer):
            return
   
        w = t.width()
        
        if abs(op.value()) < 2**(w-1):
            # Const is in the right range, reset the type.
            op.setConstant(Constant(op.value(), type.Integer(w)))
            
    return _refineIntegerConstant

def integerOfWidth(n):
    """Returns a constraint function that checks whether an operand is of an
    integer type with the given width *n*. The constraint function will also
    have a ~~refineIntegerConstant refinement associated with it."""

    @constraint("int<%d>" % n)
    @refineType(refineIntegerConstant(n))
    def _integerOfWidth(ty, op, i):
        return (isinstance(ty, type.Integer) and ty.width() == n, "must be of type int<%d> but is %s" % (n, ty))
            
    return _integerOfWidth

def integerOfWidthAsOp(n):
    """Returns a constraint function that checks whether an operand is of an
    integer type with the same width as operand *n*; 0 is the target operand.
    The constraint function will also have a ~~refineIntegerConstantWithOp 
    refinement associated with it."""

    @constraint(lambda sig: sig.getOpDoc(_getSigOp(sig, n)))
    @refineType(refineIntegerConstantWithOp(n))
    def _isIntOfSameWidthAs(ty, op, i):
        w = _getOp(i, n).type().width()
        return (isinstance(ty, type.Integer) and ty.width() == w, \
               "must be of same integer type as %s, which is int<%d>" % (_getOpName(n), w))
        
    return _isIntOfSameWidthAs

def nonZero(constr):
    """A constraint function that ensures that a constant operand conforms
    with *constr* and is not zero. For non-constant operands,
    only *constr* is checked, the value is ignored."""
    
    @constraint(lambda sig: sig.getOpDoc(constr))
    def _nonZero(ty, op, i):
        (success, msg) = constr(ty, op, i)
        
        if not success:
            return (False, msg)
        
        if op and isinstance(op, ConstOperand):
            return (op.value() != 0, "must not be zero")
        else:
            return (True, "")
    
    return _nonZero

def referenceOf(constr):    
    """Returns a constraint function that ensures that the operand is of type
    ~~Reference and its ~~refType() conforms with *constraint*. 
    """ 
    assert_constraint(constr)
    @constraint(lambda sig: "ref<%s>" % sig.getOpDoc(constr))
    def _referenceOf(ty, op, i):
        if not isinstance(op.type(), type.Reference):
            return (False, "must be a reference")
        return constr(ty.refType(), None, i)
    
    return _referenceOf

def container(containert, itemt):    
    """Returns a constraint function that ensures that the operand is of
    container type *containert* and its ~~itemType() conforms with *itemt*. 
    """ 
    assert_constraint(containert)
    assert_constraint(itemt)
    @constraint(lambda sig: "%s<%s>" % (sig.getOpDoc(containert), sig.getOpDoc(itemt)))
    def _container(ty, op, i):
        
        (success, msg) = containert(ty, op, i)
        if not success:
            return (False, msg)
        
        (success, msg) = itemt(ty.itemType(), None, i)
        if not success:
            return (False, "wrong container item type, " + msg)
        
        return (True, "")
    
    return _container

def itemTypeOfOp(n):
    """Returns a constraint function that ensures that the operand is of a
    container's ~~itemType. The container is expected to be found in in
    operand *n*, which must be of type Reference. If *n* is zero, the
    functions examines the target operand. The constraint function will also
    have a ~~refineIntegerConstantWithOpItemType refinement associated with it.
    """
    @constraint("*item-type*")
    @refineType(refineIntegerConstantWithOpItemType(n))
    def _itemTypeOf(ty, op, i):
        it = _getOp(i, n).type().refType().itemType()
        return (it == ty, "must be of type %s but is %s" % (it, ty))
    
    return _itemTypeOf

def derefTypeOfOp(n):
    """Returns a constraint function that ensures that the operand is of a
    type matching what an iterator in operand *n* yields.  The constraint
    function will also have a ~~refineIntegerConstantWithOpDerefType
    refinement associated with it.
    """
    @constraint("*item-type*")
    @refineType(refineIntegerConstantWithOpDerefType(n))
    def _itemTypeOf(ty, op, i):
        it = _getOp(i, n).type().containerType().itemType()
        return (it == ty, "must be of type %s but is %s" % (it, ty))
    
    return _itemTypeOf

def sameTypeAsOp(n):
    """Returns a constraint function that ensures that the operand is of the
    same type as operand *n*. If *n* is zero, the functions compares with the
    target operand. 
    """
    @constraint(lambda sig: sig.getOpDoc(_getSigOp(sig, n)))
    def _hasSameTypeAs(ty, op, i):
        o = _getOp(i, n)
        return (o.type() == ty, "type must be the same as that of %s, which is %s" % (_getOpName(n), o.type()))
    
    return _hasSameTypeAs

def isTuple(tuple):
    """Returns a constraint function that ensures that the operand is a tuple
    and each tuple element complies with a corresponding constraint function
    in *tuple*.
    """

    @constraint(lambda sig: "(" + ",".join([sig.getOpDoc(t) for t in tuple]) + ")")
    def _isTuple(ty, op, i):
        if not isinstance(ty, type.Tuple):
            return (False, "must be a tuple")
            
        if len(ty.types()) != len(tuple):
            return (False, "tuple must be of length %d" % len(tuple))
            
        for (j, t) in enumerate(ty.types()):
            (success, msg) = tuple[j](t, None, i)
            if not success:
                return (False, "tuple element %d: %s" % (j, msg))
        
        return (True, "")
    
    return _isTuple

def referenceOfOp(n):    
    """Returns a constraint function that ensures that the operand is of type
    ~~Reference and its ~~refType() is the type of operand *n* (zero is
    target). If operand *n* is a ~~TypleDeclType, it's ~~declType is taken for
    comparision.
    """ 
    @constraint(lambda sig: "ref<%s>" % sig.getOpDoc(_getSigOp(sig, n)))
    def _refOfSameTypeAs(ty, op, i):
        o =_getOp(i, n)
        
        t = o.type()
        if isinstance(t, type.TypeDeclType):
            t = t.declType()

        return (isinstance(ty, type.Reference) and ty.refType() == t, "type must be ref<%s>" % t)
            
    return _refOfSameTypeAs

def _hasType(cls, name=None):
    
    if not name:
        name = cls._name
    
    @constraint(name)
    def __hasType(ty, op, i):
        return (issubclass((ty.__class__), cls), "must be of type %s but is %s" % (name, ty))

    __hasType.__doc__ = """A constraint function that ensures that
    an operand's type is of class ~~%s""" % cls.__name__
    
    return __hasType

# Add one constaint per type here. 

string = _hasType(type.String)
integer = _hasType(type.Integer)
bytes = _hasType(type.Bytes)
iteratorBytes = _hasType(type.IteratorBytes)
channel = _hasType(type.Channel)
double = _hasType(type.Double)
bool = _hasType(type.Bool)
any = _hasType(type.Type, "any")
enum = _hasType(type.Enum)
reference = _hasType(type.Reference)
struct = _hasType(type.Struct)
tuple = _hasType(type.Tuple)
addr =  _hasType(type.Addr)
net =  _hasType(type.Net)
port =  _hasType(type.Port)
overlay =  _hasType(type.Overlay)
vector =  _hasType(type.Vector)
iteratorVector =  _hasType(type.IteratorVector)
list =  _hasType(type.List)
iteratorList =  _hasType(type.IteratorList)
regexp =  _hasType(type.RegExp)
bitset =  _hasType(type.Bitset)
exception =  _hasType(type.Exception)
caddr =  _hasType(type.CAddr)

function = _hasType(type.Function)
label = _hasType(type.Label)
hiltiType = _hasType(type.HiltiType)
heapType = _hasType(type.HeapType)
valueType = _hasType(type.ValueType)







# $Id$
"""The ``constraints`` module predefines the following constraint
functions. See :ref:`signature-constraints` for more information
about such functions. Note that some of these functions are actually
not constraint function themselves but *return* constraint functions
when called. 
""" 

import constant
import hlt
import type
import id
import instruction
import operand

Constant = constant.Constant # We'll override constant() later. 

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

def cOptional(constr):
    """Returns a constraint function that checks whether an operand is either
    not in place or, if given, conforms with another *constraint* function."""
    instruction.assert_constraint(constr)
    @hlt.constraint(lambda sig: "[%s]" % sig.getOpDoc(constr))
    def _optional(ty, op, i):
        return constr(ty, op, i) if op else (True, "")

    # This is a hack to mark the optional() costraint.
    _optional._optional = True
    return _optional
    
def cConstant(constr):
    """Returns a constraint function that checks whether an operand is a
    constant conforming with the additional constraint function *constraint*."""
    instruction.assert_constraint(constr)
    @hlt.constraint(lambda sig: sig.getOpDoc(constr))
    def _isConstant(ty, op, i):
        return constr(ty, op, i) if (op and isinstance(op, operand.Constant)) else (False, "must be a constant")
        
    return _isConstant

def cType(constr):
    """Returns a constraint function that checks whether an operand is a
    ~~Type, or an ~~ID referecing a type, conforming with
    *constraint*."""
    instruction.assert_constraint(constr)
    @hlt.constraint(lambda sig: "%s" % sig.getOpDoc(constr))
    def _isType(ty, op, i):
        if not op:
            return (False, "missing")

        if not isinstance(op, operand.Type) and \
           (not isinstance(op, operand.ID) or not isinstance(op.value(), id.Type)):
               return (False, "must be a type operand")

        r = None
        if isinstance(op, operand.Type):
            t = op.value()
            
        if isinstance(op, operand.ID):
            if isinstance(op.value(), id.Type):
                t = op.value().type()
        
        if not t:
            return (False, "must be a type operand")
           
        return constr(t, None, i)
        
    return _isType

def cIntegerOfWidth(n):
    """Returns a constraint function that checks whether an operand is of an
    integer type with the given width *n*. 
    """

    @hlt.constraint("int<%d>" % n)
    def _integerOfWidth(ty, op, i):
        if not ty:
            return (False, "missing")
        
        if op:
            return (isinstance(ty, type.Integer) and op.canCoerceTo(type.Integer(n)), "must be of type int<%d> but is %s" % (n, ty))
        else:
            return (isinstance(ty, type.Integer) and ty.canCoerceTo(type.Integer(n)), "must be of type int<%d> but is %s" % (n, ty))
            
    return _integerOfWidth

def cIntegerOfWidthAsOp(n):
    """Returns a constraint function that checks whether an operand is of an
    integer type with the same width as operand *n*; 0 is the target operand.
    """

    @hlt.constraint(lambda sig: sig.getOpDoc(_getSigOp(sig, n)))
    def _isIntOfSameWidthAs(ty, op, i):
        if not isinstance(ty, type.Integer):
            return (False, "must be an integer")
        
        other = _getOp(i, n)
        if not other:
            return (False, "missing")
        
        otype = other.type()
      
        if not isinstance(otype, type.Integer):
            return (False, "must be of integer type")

        if isinstance(other, operand.Constant):
            return (type.canCoerceConstantTo(other.value(), otype), "can't be coerced from integer of width %d to one of width %d" % (op.type().width(), otype.width()))
        else:
            return(op.canCoerceTo(otype), "can't be coerced from integer of width %d to one of width %d" % (op.type().width(), otype.width()))

    return _isIntOfSameWidthAs

def cNonZero(constr):
    """A constraint function that ensures that a constant operand conforms
    with *constr* and is not zero. For non-constant operands,
    only *constr* is checked, the value is ignored."""
    
    @hlt.constraint(lambda sig: sig.getOpDoc(constr))
    def _nonZero(ty, op, i):
        (success, msg) = constr(ty, op, i)
        
        if not success:
            return (False, msg)
        
        if op and isinstance(op, operand.Constant):
            return (op.value().value() != 0, "must not be zero")
        else:
            return (True, "")
    
    return _nonZero

def cReferenceOf(constr):    
    """Returns a constraint function that ensures that the operand is of type
    ~~Reference and its ~~refType() conforms with *constraint*. 
    """ 
    instruction.assert_constraint(constr)
    @hlt.constraint(lambda sig: "ref<%s>" % sig.getOpDoc(constr))
    def _referenceOf(ty, op, i):
        if not isinstance(ty, type.Reference):
            return (False, "must be a reference")

        return constr(ty.refType(), None, i)
    
    return _referenceOf

def cContainer(containert, itemt):    
    """Returns a constraint function that ensures that the operand is of
    container type *containert* and its ~~itemType() conforms with *itemt*. 
    """ 
    instruction.assert_constraint(containert)
    instruction.assert_constraint(itemt)
    @hlt.constraint(lambda sig: "%s<%s>" % (sig.getOpDoc(containert), sig.getOpDoc(itemt)))
    def _container(ty, op, i):
        
        (success, msg) = containert(ty, op, i)
        if not success:
            return (False, msg)
        
        (success, msg) = itemt(ty.itemType(), None, i)
        if not success:
            return (False, "wrong container item type, " + msg)
        
        return (True, "")
    
    return _container

def cItemTypeOfOp(n):
    """Returns a constraint function that ensures that the operand is of a
    container's ~~itemType. The container is expected to be found in in
    operand *n*, which must be of type Reference. If *n* is zero, the
    functions examines the target operand.
    """
    @hlt.constraint("*item-type*")
    def _itemTypeOf(ty, op, i):
        it = _getOp(i, n).type().refType().itemType()
        return (op.canCoerceTo(it), "must be of type %s but is %s" % (it, ty))
    
    return _itemTypeOf

def cDerefTypeOfOp(n):
    """Returns a constraint function that ensures that the operand is of a
    type matching what an iterator in operand *n* yields.
    """
    @hlt.constraint("*item-type*")
    def _itemTypeOf(ty, op, i):
        it = _getOp(i, n).type().derefType()
        return (op.canCoerceTo(it), "must be of type %s but is %s" % (it, ty))
    
    return _itemTypeOf

def cSameTypeAsOp(n):
    """Returns a constraint function that ensures that the operand is of the
    same type as operand *n*. If *n* is zero, the functions compares with the
    target operand. 
    """
    @hlt.constraint(lambda sig: sig.getOpDoc(_getSigOp(sig, n)))
    def _hasSameTypeAs(ty, op, i):
        o = _getOp(i, n)
        return (o.type() == ty, "type must be the same as that of %s, which is %s" % (_getOpName(n), o.type()))
    
    return _hasSameTypeAs

def cCanCoerceToOp(n):
    """Returns a constraint function that ensures that the operand is of a
    type that can be coerced to that of operand *n*. If *n* is zero, the
    functions compares with the target operand. 
    """
    @hlt.constraint(lambda sig: sig.getOpDoc(_getSigOp(sig, n)))
    def _hasSameTypeAs(ty, op, i):
        o = _getOp(i, n)
        return (op.canCoerceTo(o.type()), "must be coercable to type of %s, which is %s" % (_getOpName(n), o.type()))
    
    return _hasSameTypeAs

def cIsTuple(tuple):
    """Returns a constraint function that ensures that the operand is a tuple
    and each tuple element complies with a corresponding constraint function
    in *tuple*.
    """

    @hlt.constraint(lambda sig: "(" + ",".join([sig.getOpDoc(t) for t in tuple]) + ")")
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

def cReferenceOfOp(n):    
    """Returns a constraint function that ensures that the operand is of type
    ~~Reference and its ~~refType() is the type of operand *n* (zero is
    target).
    """ 
    @hlt.constraint(lambda sig: "ref<%s>" % sig.getOpDoc(_getSigOp(sig, n)))
    def _refOfSameTypeAs(ty, op, i):
        o =_getOp(i, n)
        
        if isinstance(o, operand.Type):
            t = o.value()
        else:
            t = o.type()
            
        return (isinstance(ty, type.Reference) and ty.refType() == t, "type must be ref<%s>" % t)
            
    return _refOfSameTypeAs

def _hasType(name, altname=None):
    if not altname:
        altname = name.lower()
    
    @hlt.constraint(altname)
    def __hasType(ty, op, i):
        cls = type.__dict__[name]
        return (issubclass(ty.__class__, cls), "must be of type %s but is %s" % (altname, ty))

    # FIXME: Reanable
    #__hasType.__doc__ = """A constraint function that ensures that
    #an operand's type is of class ~~%s""" % cls.__name__
    
    return __hasType

# Add one constaint per type here. We use strings to avoid a dependency on the
# automatically generated namespace entries in Type. Eventually, we should
# probably get rid of this whole list here and auto-generate these as well.

cString = _hasType("String")
cInteger = _hasType("Integer")
cBytes = _hasType("Bytes")
cIteratorBytes = _hasType("IteratorBytes", "iterator<bytes>")
cChannel = _hasType("Channel")
cDouble = _hasType("Double")
cBool = _hasType("Bool")
cAny = _hasType("Type", "any")
cEnum = _hasType("Enum")
cReference = _hasType("Reference")
cStruct = _hasType("Struct")
cTuple = _hasType("Tuple")
cAddr =  _hasType("Addr")
cNet =  _hasType("Net")
cPort =  _hasType("Port")
cOverlay =  _hasType("Overlay")
cVector =  _hasType("Vector")
cIteratorVector =  _hasType("IteratorVector", "iterator<vector>")
cList =  _hasType("List")
cIteratorList =  _hasType("IteratorList", "iterator<list>")
cRegexp =  _hasType("RegExp")
cBitset =  _hasType("Bitset")
cException =  _hasType("Exception")
cCaddr =  _hasType("CAddr")
cTimer = _hasType("Timer")
cTimerMgr = _hasType("TimerMgr")
cMap = _hasType("Map")
cSet = _hasType("Set")
cIteratorMap = _hasType("IteratorMap", "iterator<map>")
cIteratorSet = _hasType("IteratorSet", "iterator<set>")
cIOSrc = _hasType("IOSrc")
cIteratorIOSrc = _hasType("IteratorIOSrc", "iterator<iosrc>")
cFile = _hasType("File")
cCallable = _hasType("Callable")

cFunction = _hasType("Function")
cHook = _hasType("Hook")
cLabel = _hasType("Label")
cHiltiType = _hasType("HiltiType")
cHeapType = _hasType("HeapType", "heap type")
cValueType = _hasType("ValueType", "value type")
cIterator = _hasType("Iterator")
cIterable = _hasType("Iterable")

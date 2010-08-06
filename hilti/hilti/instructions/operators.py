"""
Operators
~~~~~~~~~

An *operator* is a instruction that it not tied to a particular type but 
is overloaded for arguments of different types to implement
semantically similar operations. Note that the specifics of each operation is
defined on a per-type basis, and documented there. For example, the *incr* operator (which
increments its argument by one) will add 1 to an integer yet move to the
next element for a container iterator.
"""

import hilti.constant as constant
import hilti.util as util

from hilti.instruction import *
from hilti.constraints import *

@operator("new", op1=cType(cHeapType), op2=cOptional(cAny), op3=cOptional(cAny), target=cReferenceOfOp(1))
class New(Operator):
    """
    Allocates a new instance of type *op1*.
    """
    pass

@operator("incr", op1=cHiltiType, target=cSameTypeAsOp(1))
class Incr(Operator):
    """
    Increments *op1* by one and returns the result. *op1* is not modified.
    """
    pass

@operator("deref", op1=cHiltiType, target=cHiltiType)
class Deref(Operator):
    """
    Dereferences *op1* and returns the result.
    """
    pass

@operator("equal", op1=cHiltiType, op2=cHiltiType, target=cBool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*
    """
    pass

@operator("begin", op1=cReferenceOf(cIterable), target=cIterator)
class Begin(Operator):
    """
    Returns an iterator for the starting position of a sequence.
    """
    pass

@operator("end", op1=cReferenceOf(cIterable), target=cIterator)
class End(Operator):
    """
    Returns an iterator for the end position of a sequence.
    """
    pass

# FIXME: For op2, what we really want is "is of type enum Hilti::Packed". But
# how can we express that?
def cUnpackTarget(constraint):
    """A constraint function that ensures that the operand is of the
    tuple type expected for the ~~Unpack operator's target. *t* is an
    additional constraint function for the tuple's first element.
    """ 
    return cIsTuple([constraint, cIteratorBytes])

@hlt.instruction("unpack", op1=cIsTuple([cIteratorBytes, cIteratorBytes]), op2=cEnum, op3=cOptional(cAny), target=cUnpackTarget(cAny))
class Unpack(Instruction):
    """Unpacks an instance of a particular type (as determined by *target*;
    see below) from the binary data enclosed by the iterator tuple *op1*.
    *op2* defines the binary layout as an enum of type ``Hilti::Packed`` and
    must be a constant. Depending on *op2*, *op3* is may be an additional,
    format-specific parameter with further information about the binary
    layout. The operator returns a ``tuple<T, iterator<bytes>>``, in the first
    component is the newly unpacked instance and the second component is
    locates the first bytes that has *not* been consumed anymore. 
    
    Raises ~~WouldBlock if there are not sufficient bytes available
    for unpacking the type. Can also raise other exceptions if other
    errors occur, in particular if the raw bytes are not as expected
    (and that fact can be verified).

    Note: The ``unpack`` operator uses a generic implementation able to handle all data
    types. Different from most other operators, it's implementation is not
    overloaded on a per-type based. However, each type must come with an
    ~~unpack decorated function, which the generic operator implementatin
    relies on for doing the unpacking.
    
    Note: We should define error semantics more crisply. Ideally, a
    single UnpackError should be raised in all error cases. 
    """
    def _validate(self, vld):
        super(Unpack, self)._validate(vld)

        try:
            kind = self.op2().value().value()
        except AttributeError:
            # Will be reported somewhere else.
            return 
        
        if not kind:
            vld.error(self, "operand 2 is not a constant")
            return 
        
        kind = kind.value()
        
        ttype = self.target().type().types()[0]

        if not isinstance(ttype, type.Unpackable):
            vld.error(self, "cannot unpack instances of type %s" % ttype)
        
        packed = vld.currentModule().scope().lookup("Hilti::Packed")
        
        if not packed:
            util.internal_error("type Hilti::Packed is not defined")

        if packed.type() != self.op2().type():
            vld.error(self, "operand 2 must be of type Hilti::Packed")
            return

        if not self.op2().constant():
            vld.error(self, "operand 2 must be constant")
            return
        
        found = False
        for (labels, ty, optional, descr) in ttype.formats(vld.currentModule()):
            
            if not isinstance(labels, list) or isinstance(labels, tuple):
                labels = [labels]
            
            for label in labels:
                if label == kind:
                    if ty and not optional and not self.op3():
                        vld.error(self, "non-optional operand 3 missing")
                        return 
                        
                    if not ty and self.op3():
                        vld.error(self, "superfluous operand 3 ")
                        return 
                    
                    if self.op3() and not self.op3().canCoerceTo(ty):
                        vld.error(self, "operand 3 must be of type %s but is %s" % (ty, self.op3().type()))
                        return
                        
                    found = True
                    break
            
        if not found:
            vld.error(self, "unpack type %s not supported by type %s" % (kind, ttype))
        
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        
        begin = cg.llvmExtractValue(op1, 0)
        end = cg.llvmExtractValue(op1, 1)
            
        t = self.target().type().types()[0]
        
        (val, iter) = cg.llvmUnpack(t, begin, end, self.op2(), self.op3())
        
        val = operand.LLVM(val, self.target().type().types()[0])
        iter = operand.LLVM(iter, self.target().type().types()[1])
        
        tuple = constant.Constant([val, iter], type.Tuple([type.IteratorBytes()] * 2))
        cg.llvmStoreInTarget(self, operand.Constant(tuple))
    
@hlt.instruction("assign", op1=cCanCoerceToOp(0), target=cValueType)
class Assign(Instruction):
    """Assigns *op1* to the target.
    
    There is a short-cut syntax: instead of using the standard form ``t = assign op``, one
    can just write ``t = op``.
    
    Note: The ``assign`` operator uses a generic implementation able to handle all data
    types. Different from most other operators, it's implementation is not
    overloaded on a per-type based. 
    """
    def _codegen(self, cg):
        op = cg.llvmOp(self.op1(), self.target().type())
        cg.llvmStoreInTarget(self, op)
    

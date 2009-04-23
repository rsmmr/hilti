# $Id$
"""
Integers
~~~~~~~~
"""

_doc_type_description = """
The *integer* data type represents signed integers of a fixed width. The width
is specified as part of the type name as, e.g., in ``int<16>`` for a 16-bit
integer. There are predefined shortcuts ``int8``, ``int16``, ``int32`` and
``int64``. If not explictly initialized, integers are set to zero initially.
"""

from hilti.core.type import *
from hilti.core.instruction import *
from hilti.instructions.operators import *

@overload(Unpack, op1=IteratorBytes, op2=IteratorBytes, op3=Enum, target=Tuple)
class Unpack(Operator):
    """Unpacks a string from a sequence of raw bytes; see the ~~Unpack
    operator for information about *op1*, and *op2* and *target*.
    
    *op3* specifies the layout of the bytes and can have one of the following
    values of enum type ``Hilti::Packed``:
    
    .. literalinclude:: /libhilti/hilti.hlt
       :start-after: %doc-packed-int-start
       :end-before:  %doc-packed-int-start

    For the signed variants, the bit-width of the target integer must match
    the width of the unpacket integer. However, because we don't have 
    *unsigned* integers in HILTI, for the ``Packet::UInt*`` versions, the
    width of the target integer must be *larger* than that of the unpacked
    value: for ``Packed::Uint8`` is must be ``int16`, for ``Packed::UInt8`` is
    must be ``int32`, etc.
    
    Todo: Fix the text: we differentiate between constants and non-constant
    as *op3*.
    """
    
@overload(Incr, op1=Integer, target=Integer)
class Incr(Operator):
    """
    Returns the result of ``op1 + 1``. The result is undefined if an
    overflow occurs.
    """
    pass

@overload(Equal, op1=Integer, op2=Integer, target=Bool)
class Equals(Operator):
    """
    Returns True if *op1* equals *op2*.
    """
    pass

@instruction("int.add", op1=Integer, op2=Integer, target=Integer)
class Add(Instruction):
    """
    Calculates the sum of the two operands. Operands and target must be of
    same width. The result is calculated modulo 2^{width}. 
    """

@instruction("int.sub", op1=Integer, op2=Integer, target=Integer)
class Sub(Instruction):
    """
    Subtracts *op1* one from *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """
    
@instruction("int.mul", op1=Integer, op2=Integer, target=Integer)
class Mul(Instruction):
    """
    Multiplies *op1* with *op2*. Operands and target must be of same width.
    The result is calculated modulo 2^{width}.
    """

@instruction("int.div", op1=Integer, op2=Integer, target=Integer)
class Div(Instruction):
    """
    Divides *op1* by *op2*, flooring the result. Operands and target must be
    of same width.  If the product overflows the range of the integer type,
    the result in undefined. 
    
    Throws :exc:`DivisionByZero` if *op2* is zero.
    """
    
@instruction("int.eq", op1=Integer, op2=Integer, target=Bool)
class Eq(Instruction):
    """
    Returns true iff *op1* equals *op2*. 
    """
    
@instruction("int.lt", op1=Integer, op2=Integer, target=Bool)
class Lt(Instruction):
    """
    Returns true iff *op1* is less than *op2*.
    """

@instruction("int.ext", op1=Integer, target=Integer)
class Ext(Instruction):
    """
    Zero-extends *op1* into an integer of the same width as the *target*. The
    width of *op1* must be smaller or equal that of the *target*. 
    """

@instruction("int.trunc", op1=Integer, target=Integer)
class Trunc(Instruction):
    """
    Bit-truncates *op1* into an integer of the same width as the *target*. The
    width of *op1* must be larger or equal that of the *target*. 
    """

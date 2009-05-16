# $Id$
"""
Strings
~~~~~~~
"""

_doc_type_description = """Strings are sequences of characters and are
intended to be primarily used for text that at some point might be presented
to a human in one way or the other. They are inmutable, copied by value on
assignment, and internally stored in UTF-8 encoding. Don't use it for larger
amounts of binary data, performance won't be good.

Strings constants are written in the usual form ``"Foo"``. They are assumed to
be in 7-bit ASCII encoding; usage of 8-bit characters is undefined at the
moment.

If not explictly initialized, strings are set to empty initially.

Todo: 
* Explain control sequences in string constants. 
* Add some way to define the encoding for constants.

"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@constraint('"ascii"|"utf8"')
def _encoding(ty, op, i):
    if not isinstance(ty, type.String):
        return (False, "encoding must be a string")
    
    if op and isinstance(op, ConstOperand):
        return (op.value() in ("ascii", "utf8"), "unknown charset %s" % op.value())
    
    return (True, "")

@instruction("string.length", op1=string, target=integerOfWidth(32))
class Length(Instruction):
    """Returns the number of characters in the string *op1*."""
    pass

@instruction("string.concat", op1=string, op2=string, target=string)
class Concat(Instruction):
    """Concatenates *op1* with *op2* and returns the result."""
    pass

@instruction("string.substr", op1=string, op2=integerOfWidth(32), op3=integerOfWidth(32), target=string)
class Substr(Instruction):
    """Extracts the substring of length *op3* from *op1* that starts at
    position *op2*."""
    pass

@instruction("string.find", op1=string, op2=string, target=integerOfWidth(32))
class Find(Instruction):
    """Searches *op2* in *op1*, returning the start index if it find it.
    Returns -1 if it does not find *op2* in *op1*."""
    pass

@instruction("string.cmp", op1=string, op2=string, target=bool)
class Cmp(Instruction):
    """Compares *op1* with *op2* and returns True if their characters match.
    Returns False otherwise."""
    pass

@instruction("string.lt", op1=string, op2=string, target=bool)
class Lt(Instruction):
    """Compares *op1* with *op2* and returns True if *op1* is
    lexicographically smaller than *op2*. Returns False otherwise."""
    pass

@instruction("string.decode", op1=referenceOf(bytes), op2=_encoding, target=string)
class Decode(Instruction):
    """ 
    Converts *bytes op1* into a string, assuming characters are encoded in
    character set *op2*. Supported character sets are currently: ``ascii``,
    ``utf8``. 
    
    If the string cannot be decoded with the given character set, the
    instruction throws an ~~DecodingError exception. If the character set
    given is not known, the instruction throws a ~~ValueError exception. 
    
    Todo: We need to figure out how to support more character sets.
    """
    pass

@instruction("string.encode", op1=string, op2=_encoding, target=referenceOf(bytes))
class Encode(Instruction):
    """Converts *op1* into bytes, encoding characters using the character set
    *op2*. Supported character sets are currently: ``ascii``, ``utf8``. 
    
    If the any characters cannot be encoded with the given character set, they
    will be replaced with place-holders. If the character set given is not
    known, the instruction throws a ~~ValueError exception. 
    
    Todo: We need to figure out how to support more character sets.
    """
    pass

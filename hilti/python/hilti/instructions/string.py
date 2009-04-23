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
from hilti.core.type import *
from hilti.core.instruction import *
from hilti.instructions.operators import *

@overload(Unpack, op1=IteratorBytes, op2=IteratorBytes, op3=Tuple, target=Tuple)
class Unpack(Operator):
    """
    Unpacks a string from a sequence of raw bytes; see the ~~Unpack operator
    for information about *op1*, and *op2* and *target*.
    
    *op3* is a tuple of type ``tuple<Charset, Packed, Any>. The first component of
    the tuple specifies the character set used to decode the bytes; the
    second component defines how the length of the string is determined. It
    can either be ``Packed::ASCIIZ`` or any of the ``Packed::*`` values
    supported by the integer type's ~~integer.Unpack operator. In the former case
    (``Packed::ASCIIZ``), the string will ends with the first null byte found
    (or *op2*, whatever comes first). In the latter case, the raw bytes are
    assumed to start with a length field indicating the number of bytes that
    are part of the string, and the length field is unpacked according to
    ``Packed::*`` type. 
    """
    pass

@instruction("string.assign", op1=String, target=String)
class Assign(Instruction):
    """Assigns *op1* to the target."""
    pass

@instruction("string.length", op1=String, target=Integer(32))
class Length(Instruction):
    """Returns the number of characters in the string *op1*."""
    pass

@instruction("string.concat", op1=String, op2=String, target=String)
class Concat(Instruction):
    """Concatenates *op1* with *op2* and returns the result."""
    pass

@instruction("string.substr", op1=String, op2=Integer, op3=Integer, target=String)
class Substr(Instruction):
    """Extracts the substring of length *op3* from *op1* that starts at
    position *op2*."""
    pass

@instruction("string.find", op1=String, op2=String, target=Integer)
class Find(Instruction):
    """Searches *op2* in *op1*, returning the start index if it find it.
    Returns -1 if it does not find *op2* in *op1*."""
    pass

@instruction("string.cmp", op1=String, op2=String, target=Bool)
class Cmp(Instruction):
    """Compares *op1* with *op2* and returns True if their characters match.
    Returns False otherwise."""
    pass

@instruction("string.lt", op1=String, op2=String, target=Bool)
class Lt(Instruction):
    """Compares *op1* with *op2* and returns True if *op1* is
    lexicographically smaller than *op2*. Returns False otherwise."""
    pass

@instruction("string.decode", op1=Reference, op2=String, target=String)
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

@instruction("string.encode", op1=String, op2=String, target=Reference)
class Encode(Instruction):
    """Converts *op1* into bytes, encoding characters using the character set
    *op2*. Supported character sets are currently: ``ascii``, ``utf8``. 
    
    If the any characters cannot be encoded with the given character set, they
    will be replaced with place-holders. If the character set given is not
    known, the instruction throws a ~~ValueError exception. 
    
    Todo: We need to figure out how to support more character sets.
    """
    pass

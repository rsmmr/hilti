# $Id$
"""Strings are sequences of characters and are intended to be primarily used
for text that at some point might be presented to a human in one way or the
other. They are inmutable, copied by value on assignment, and internally
stored in UTF-8 encoding. Don't use it for larger amounts of binary data,
performance won't be good.

Strings constants are written in the usual form ``"Foo"``. They are assumed to
be in 7-bit ASCII encoding; usage of 8-bit characters is undefined at the
moment.

Todo: 
* Explain control sequences in string constants. 
* Add some way to define the encoding for constants.

"""
from hilti.core.type import *
from hilti.core.instruction import *

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
class Find(Instruction):
    """Compares *op1* with *op2* and returns True if their characters match.
    Returns False otherwise."""
    pass

@instruction("string.sprintf", op1=String, op2=Tuple, target=String)
class Sprintf(Instruction):
    """Constructs a new string according to the printf-style format
    string *op1*, using *op2* to fill in format symbols.
    
    Todo: Explain format symbols.
    """
    pass

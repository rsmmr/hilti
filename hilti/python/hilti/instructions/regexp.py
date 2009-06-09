# $Id$
"""
Regular Expressions
~~~~~~~~~~~~~~~~~~~
"""

_doc_type_description = """The *regexp* data type provides regular expression
matching on *string* and *bytes* objects.
"""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@constraint("string | iterator<bytes>")
def _string_or_iter(ty, op, i):
    if not op:
        return (False, "operand missing")
    
    if not isinstance(ty, type.String) and not isinstance(ty, type.IteratorBytes):
        return (False, "operand must be string or iterator<byte>")
    
    return (True, "")

@constraint("None | iterator<bytes>")
def _none_or_iter(ty, op, i):
    if isinstance(i.op2().type(), type.String):
        return (op == None, "superflous operand")
    
    return (isinstance(ty, type.IteratorBytes), "must be of type iterator<bytes>")

@overload(New, op1=isType(regexp), target=referenceOf(regexp))
class New(Operator):
    """
    Instantiates a new *regexp* instance. Before any of the matching
    functionality can be used, ~~Compile must be used to compile a pattern.
    """
    pass

@instruction("regexp.compile", op1=referenceOf(regexp), op2=string)
class Compile(Instruction):
    """Compiles the pattern in *op2* for subsequent matching. The string in
    *op2* must only contain ASCII characters and must not contain any
    back-references. 
    
    Todo: We should support other than ASCII characters too but need the
    notion of a local character set first. We should also support
    back-references. (Our RE library does but we need to implement a bit more
    C code for that (rewind & compare for reguexec).
    """
    pass

@instruction("regexp.find", op1=referenceOf(regexp), op2=_string_or_iter, op3=_none_or_iter, target=bool)
class Find(Instruction):
    """
    Scans either the string in *op1* or the byte iterator range between *op2*
    and *op3* for the regular expression op1*. Returns True if found,
    otherwise False.
    
    Throws ~~PatternError if not pattern has been compiled yet. 
    
    Todo: The string variant is not yet implemented.
    """
    
_span = isTuple([iteratorBytes] * 2)
    
@instruction("regexp.span", op1=referenceOf(regexp), op2=_string_or_iter, op3=_none_or_iter, target=_span)
class Span(Instruction):
    """Scans either the string in *op1* or the byte iterator range
    between *op2* and *op3* for the regular expression *op1*. If the
    regular expression is found, returns either a string with the
    match or a tuple of iterators locating the bytes which match,
    respectively. If the regular is not found, returns either an
    empty string or a tuple with two ``bytes.end`` iterators,
    respectively.
    
    Throws PatternError if no pattern has been compiled yet. 
    
    Todo: The string variant is not yet implemented.
    """
    
@instruction("regexp.groups", op1=referenceOf(regexp), op2=_string_or_iter, op3=_none_or_iter, target=referenceOf(container(vector, _span)))
class Groups(Instruction):
    """Scans either the string in *op1* or the byte iterator range
    between *op2* and *op3* for the regular expression op1*. If the
    regular expression is found, returns a vector with one entry for
    each group defined in the regular expression. Each entry is
    either the substring matching the corresponding subexpression or
    a range of iterators locating the matching bytes, respectively.
    Index 0 always contains the string/bytes that match the total
    expression. Returns an empty vector if the expression is not
    found.
    
    Throws PatternError if not pattern has been compiled yet. 
    
    Todo: The string variant is not yet implemented.
    """


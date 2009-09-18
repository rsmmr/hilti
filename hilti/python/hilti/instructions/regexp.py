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

@constraint("string | ref<list<string>>")
def _string_or_list(ty, op, i):
    if isinstance(ty, type.String):
        return (True, "")
    
    if isinstance(ty, type.Reference) and \
       isinstance(ty.refType(), type.List) and \
       isinstance(ty.refType().itemType(), type.String):
        return (True, "")
    
    return (False, "must be string or list<string>")
       
@overload(New, op1=isType(regexp), target=referenceOf(regexp))
class New(Operator):
    """
    Instantiates a new *regexp* instance. Before any of the matching
    functionality can be used, ~~Compile must be used to compile a pattern.
    """
    pass

@instruction("regexp.compile", op1=referenceOf(regexp), op2=_string_or_list)
class Compile(Instruction):
    """Compiles the pattern(s) in *op2* for subsequent matching.
    *op2* can be either a string with a single pattern, or a list of
    strings with a set of patterns. All patterns must only contain
    ASCII characters and must not contain any back-references. 
    
    Each pattern instance can be compiled only once. Throws ~~ValueError if a
    second compilation attempt is performed. 
    
    Todo: We should support other than ASCII characters too but need the
    notion of a local character set first. We should also support
    back-references. (Our RE library does but we need to implement a bit more
    C code for that (rewind & compare for reguexec).
    """
    pass

@instruction("regexp.find", op1=referenceOf(regexp), op2=_string_or_iter, op3=_none_or_iter, target=integerOfWidth(32))
class Find(Instruction):
    """Scans either the string in *op1* or the byte iterator range between
    *op2* and *op3* for the regular expression *op1*. Returns a positive
    integer if a match found found; if a set of patterns has been compiled,
    the integer then indicates which pattern has matched. If multiple
    patterns from the set match, the left-most one is taken. If multiple
    patterns match at the left-most position, it is undefined which of them is
    returned. The instruction returns 0 if no match was found but adding more
    input bytes could change that (i.e., a partial match). Returns -1 if no
    match was found and adding more input would not change that. 
    
    Todo: The string variant is not yet implemented.
    """
    
_range = isTuple([iteratorBytes] * 2)
_span = isTuple([integerOfWidth(32), _range])
    
@instruction("regexp.span", op1=referenceOf(regexp), op2=_string_or_iter, op3=_none_or_iter, target=_span)
class Span(Instruction):
    """Scans either the string in *op1* or the byte iterator range between
    *op2* and *op3* for the regular expression *op1*. Returns a 2-tuple with
    (1) a integer match-indicator corresponding to the one returned by ~~Find;
    and (2) the matching substring or a tuple of iterators locating the bytes
    which match, respectively; if there's no match, the second element is
    either an empty string or a tuple with two ``bytes.end`` iterators,
    respectively.
    
    Throws PatternError if no pattern has been compiled yet. 
    
    Todo: The string variant is not yet implemented.
    """
    
@instruction("regexp.groups", op1=referenceOf(regexp), op2=_string_or_iter, op3=_none_or_iter, target=referenceOf(container(vector, _range)))
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
    
    This method is not compatible with sets of multiple patterns;
    throws PatternError if used with a set, or when no pattern has
    been compiled yet. 
    
    Todo: The string variant is not yet implemented.
    """


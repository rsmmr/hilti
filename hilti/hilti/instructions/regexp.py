# $Id$
"""
Regular Expressions
~~~~~~~~~~~~~~~~~~~

The *regexp* data type provides regular expression matching on *string* and
*bytes* objects.
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

import bytes

@hlt.type("regexp", 18)
class RegExp(type.HeapType, type.Constructable, type.Parameterizable):
    """Type for ``regexp``.

    attrs: list containing strings - The set of compilation attributes applying
    to this type; can be empty if none. Currently defined is only ``&nosub`` to
    specify that no subgroup capturing will be used.

    Note: The attributes are internally converted into a single integer
    containing the bitmask as expected by libhilti.

    Todo: Not sure this attribute handling is great ... If more types need
    something like this, we should come up with a nicer solution.
    """

    # This table maps flag attributes to their decimal values. Note that the
    # value must correspond to what libhilti defines in regexp.h.
    _attrs = {
        "<wildcard>": 0,
        "&nosub": 1
        }

    def __init__(self, attrs=[], location=None):
        super(RegExp, self).__init__(location=location)
        self._attrs = attrs

    def attrs(self):
        """Returns the regexp's compilation attributes.

        attrs: list containing strings - The set of compilation attributes applying
        to this type; can be empty if none.
        """
        return self._attrs

    ### Overridden from Type.

    def name(self):
        return "regexp<%s>" % ",".join(self._attrs)

    def _validate(self, vld):
        for attr in self._attrs:
            if not attr in RegExp._attrs:
                vld.error(self, "unknown regexp attribute %s" % attr)

    def cmpWithSameType(self, other):
        # We ignore differences in attributes.
        return True

    def output(self, printer):
        printer.output(self.name())

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """A ``regexp`` object is mapped to ``hlt_regexp *``."""
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_regexp *"
        typeinfo.to_string = "hlt::regexp_to_string"
        return typeinfo

    ### Overridden from Constructable.

    def validateCtor(self, cg, val):
        """Todo."""
        def is_list(l):
            return isinstance(val, list) or isinstance(val, tuple)

        if not is_list(val):
            util.internal_error("ctor value for regexp must be list of tuple (list of string, list of string)")

        if not is_list(val) or len(val) != 2:
            util.internal_error("ctor value must contain elements of tuple (list of string, list of string)")

        (pats, attrs) = val

        if not is_list(pats):
            util.internal_error("ctor value must contains elements of tuple (list of string, list of string), but idx 0 of one is something different")

        for p in pats:
            if not isinstance(p, str):
                util.internal_error("ctor value must contains patterns as strings")

        if not is_list(attrs):
            util.internal_error("ctor value must contains elements of tuple (string, list of string), but idx 1 of one is something different")

        for a in attrs:
            if not isinstance(a, str):
                util.internal_error("ctor value must contains attributes as strings")

            if not a in _attrs:
                util.internal_error("ctor value must contains undefined")

    def llvmCtor(self, cg, val):
        (patterns, attrs) = val

        ref_type = type.Reference(type.RegExp(attrs))
        top = operand.Type(ref_type)
        regexp = cg.llvmCallC("hlt::regexp_new", [top], abort_on_except=True)
        regexp_op = operand.LLVM(regexp, ref_type)

        if len(patterns) == 1:
            # Just one pattern. We use regexp_compile().
            arg = operand.Constant(constant.Constant(patterns[0], type.String()))

            cg.llvmCallC("hlt::regexp_compile", [regexp_op, arg], abort_on_except=True)

        else:
            # More than one pattern. We built a list of the patterns and then
            # call compile_set.
            item_type = type.String()
            list_type = type.Reference(type.List(item_type))
            list = cg.llvmCallC("hlt::list_new", [operand.Type(item_type)], abort_on_except=True)

            for pat in patterns:
                item = operand.Constant(constant.Constant(pat, item_type))
                list_op = operand.LLVM(list, list_type)
                cg.llvmCallC("hlt::list_push_back", [list_op, item], abort_on_except=True)

            list_op = operand.LLVM(list, list_type)
            cg.llvmCallC("hlt::regexp_compile_set", [regexp_op, list_op], abort_on_except=True)

        return regexp

    def outputCtor(self, printer, val):
        (patterns, attrs) = val

        patterns = " | ".join(["/%s/" % p for p in patterns])
        attrs = "".join([" %s" % a for a in attrs])

        printer.output(patterns + attrs)

    def canCoerceCtorTo(self, ctor, dsttype):
        return isinstance(dsttype, type.RegExp)

    def coerceCtorTo(self, cg, ctor, dsttype):
        assert self.canCoerceCtorTo(ctor, dsttype)
        return [ctor[0], dsttype.attrs()]

    ### Overridden from Parameterizable.

    def args(self):
        flags = 0
        for attr in self._attrs:
            flags += RegExp._attrs[attr]

        return [flags]

@hlt.constraint("string | iterator<bytes>")
def _string_or_iter(ty, op, i):
    if not op:
        return (False, "operand missing")

    if not isinstance(ty, type.String) and not isinstance(ty, type.IteratorBytes):
        return (False, "operand must be string or iterator<byte>")

    return (True, "")

@hlt.constraint("None | iterator<bytes>")
def _none_or_iter(ty, op, i):
    if isinstance(i.op2().type(), type.String):
        return (op == None, "superflous operand")

    return (isinstance(ty, type.IteratorBytes), "must be of type iterator<bytes>")

@hlt.constraint("string | ref<list<string>>")
def _string_or_list(ty, op, i):
    if isinstance(ty, type.String):
        return (True, "")

    if isinstance(ty, type.Reference) and \
       isinstance(ty.refType(), type.List) and \
       isinstance(ty.refType().itemType(), type.String):
        return (True, "")

    return (False, "must be string or list<string>")

_token = cIsTuple([cIntegerOfWidth(32), cIteratorBytes])
_range = cIsTuple([cIteratorBytes] * 2)
_span = cIsTuple([cIntegerOfWidth(32), _range])

@hlt.overload(New, op1=cType(cRegexp), target=cReferenceOf(cRegexp))
class New(Operator):
    """
    Instantiates a new *regexp* instance. Before any of the matching
    functionality can be used, ~~Compile must be used to compile a pattern.
    """
    def _codegen(self, cg):
        top = operand.Type(self.op1().value())
        result = cg.llvmCallC("hlt::regexp_new", [top])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("regexp.compile", op1=cReferenceOf(cRegexp), op2=_string_or_list)
class Compile(Instruction):
    """Compiles the pattern(s) in *op2* for subsequent matching.
    *op2* can be either a string with a single pattern, or a list of
    strings with a set of patterns. All patterns must only contain
    ASCII characters and must not contain any back-references.

    Each pattern instance can be compiled only once. Throws ~~ValueError if a
    second compilation attempt is performed.

    Todo: We should support other than ASCII characters too but need the
    notion of a local character set first.
    """
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            cg.llvmCallC("hlt::regexp_compile", [self.op1(), self.op2()])
        else:
            cg.llvmCallC("hlt::regexp_compile_set", [self.op1(), self.op2()])

@hlt.instruction("regexp.find", op1=cReferenceOf(cRegexp), op2=_string_or_iter, op3=cOptional(_none_or_iter), target=cIntegerOfWidth(32))
class Find(Instruction):
    """Scans either the string in *op1* or the byte iterator range between
    *op2* and *op3* for the regular expression *op1* (if op3 is not given,
    searches until the end of the bytes object). Returns a positive integer if
    a match found found; if a set of patterns has been compiled, the integer
    then indicates which pattern has matched. If multiple patterns from the
    set match, the left-most one is taken. If multiple patterns match at the
    left-most position, it is undefined which of them is returned. The
    instruction returns -1 if no match was found but adding more input bytes
    could change that (i.e., a partial match). Returns 0 if no match was found
    and adding more input would not change that.

    Todo: The string variant is not yet implemented.
    """
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_find", [self.op1(), self.op2()])
        else:
            op3 = self.op3() if self.op3() else operand.LLVM(bytes.llvmEnd(cg), type.IteratorBytes())
            result = cg.llvmCallC("hlt::regexp_bytes_find", [self.op1(), self.op2(), op3])

        cg.llvmStoreInTarget(self, result)

@hlt.instruction("regexp.match_token", op1=cReferenceOf(cRegexp), op2=_string_or_iter, op3=cOptional(_none_or_iter), target=_token)
class MatchToken(Instruction):
    """Matches the beginning of either the string in *op1* or the byte
    iterator range between *op2* and *op3* for the regular expression *op1*
    (if op3 is not given, searches until the end of the bytes object). Returns
    a 2-tuple with (1) a integer match-indicator corresponding to the one
    returned by ~~Find; and (2) a bytes iterator that, if a match has been
    found, points to the first position after the match; if there's no match,
    the second element is undefined.

    The regexp must have been compiled with the ~~NoSub attribute.

    Note: As the name implies, this a specialized version for parsing
    purposes, enabling optimizing for the case that we don't need any
    subexpression capturing and must start the match right at the initial
    position. Internally, the implementation is only slightly optimized at the
    moment but it could be improved further at some point.

    Todo: The string variant is not yet implemented. The bytes implementation
    should be further optimized.
    """
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_match_token", [self.op1(), self.op2()])
        else:
            # Bytes version.
            op3 = self.op3() if self.op3() else operand.LLVM(bytes.llvmEnd(cg), type.IteratorBytes())
            result = cg.llvmCallC("hlt::regexp_bytes_match_token", [self.op1(), self.op2(), op3])

        cg.llvmStoreInTarget(self, result)

@hlt.instruction("regexp.span", op1=cReferenceOf(cRegexp), op2=_string_or_iter, op3=_none_or_iter, target=_span)
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
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_span", [self.op1(), self.op2()])
        else:
            # Bytes version.
            result = cg.llvmCallC("hlt::regexp_bytes_span", [self.op1(), self.op2(), self.op3()])

        cg.llvmStoreInTarget(self, result)

@hlt.instruction("regexp.groups", op1=cReferenceOf(cRegexp), op2=_string_or_iter, op3=_none_or_iter, target=cReferenceOf(cContainer(cVector, _range)))
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
    def _codegen(self, cg):
        if isinstance(self.op2().type(), type.String):
            # String version.
            result = cg.llvmCallC("hlt::regexp_string_groups", [self.op1(), self.op2()])
        else:
            # Bytes version.
            result = cg.llvmCallC("hlt::regexp_bytes_groups", [self.op1(), self.op2(), self.op3()])

        cg.llvmStoreInTarget(self, result)


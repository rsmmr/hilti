# $Id$
"""
.. hlt:type:: string

   Strings are sequences of characters and are intended to be used primarily
   for text that at some point might be presented to a human in one way or the
   other. They are inmutable, copied by value on assignment, and internally
   stored in UTF-8 encoding. Don't use it for larger amounts of binary data,
   performance won't be good (use *bytes* instead).
"""

import llvm.core

import hilti.system as system
import hilti.type as type
import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *


def _llvmStringType(len=0):
    return llvm.core.Type.packed_struct([llvm.core.Type.int(64), llvm.core.Type.array(llvm.core.Type.int(8), len)])

def _llvmStringTypePtr(len=0):
    return llvm.core.Type.pointer(_llvmStringType(len))

def _makeLLVMString(cg, s):
    if not s:
        # The empty string.
        return llvm.core.Constant.null(_llvmStringTypePtr(0))

    s = s.encode("utf-8")

    def makeConst():
        size = llvm.core.Constant.int(llvm.core.Type.int(64), len(s))
        bytes = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in s]
        struct = llvm.core.Constant.packed_struct([size, llvm.core.Constant.array(llvm.core.Type.int(8), bytes)])

        glob = cg.llvmNewGlobalConst(cg.nameNewGlobal("string"), struct)

        # We need to cast the const, which has a specific array length, to the
        # actual string type, which has unspecified array length.
        return glob.bitcast(_llvmStringTypePtr())

    return cg.cache("string-const-%s" % s, makeConst)

@hlt.type("string", 4, c="hlt_string")
class String(type.ValueType, type.Constable):
    """Type for strings."""
    def __init__(self):
        super(String, self).__init__()

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return _llvmStringTypePtr()

    ### Overridden from ValueType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::string_to_string"
        typeinfo.hash = "hlt::string_hash"
        typeinfo.equal = "hlt::string_equal"
        return typeinfo

    def llvmDefault(self, cg):
        """Strings are initially empty."""
        return llvm.core.Constant.null(_llvmStringTypePtr(0))

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        assert isinstance(const.value(), str) or isinstance(const.value(), unicode)

    def llvmConstant(self, cg, const):
        """Strings constants are written in the usual form ``"Foo"``. They are
        assumed to be in 7-bit ASCII encoding; usage of 8-bit characters is
        undefined at the moment.

        Todo: Explain control sequences in string constants.
        Todo: Add some way to define the encoding for constants.
        """
        return _makeLLVMString(cg, const.value())

    def outputConstant(self, printer, const):
        printer.output('"%s"' % util.unicode_escape(const.value()))

@hlt.constraint('"ascii"|"utf8"')
def _encoding(ty, op, i):
    if not isinstance(ty, type.String):
        return (False, "encoding must be a string")

    if op and isinstance(op, operand.Constant):
        return (op.value().value() in ("ascii", "utf8"), "unknown charset %s" % op.value())

    return (True, "")

@hlt.overload(Equal, op1=cString, op2=cString, target=cBool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*.
    """

    def _codegen(self, cg):
        cmp = cg.llvmCallC("hlt::string_cmp", [self.op1(), self.op2()])
        result = cg.builder().icmp(llvm.core.IPRED_EQ, cmp, cg.llvmConstInt(0, 8))
        cg.llvmStoreInTarget(self, result)


@hlt.instruction("string.length", op1=cString, target=cIntegerOfWidth(64))
class Length(Instruction):
    """Returns the number of characters in the string *op1*."""

    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::string_len", [self.op1()], [self.op1().type()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("string.concat", op1=cString, op2=cString, target=cString)
class Concat(Instruction):
    """Concatenates *op1* with *op2* and returns the result."""

    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::string_concat", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("string.substr", op1=cString, op2=cIntegerOfWidth(64), op3=cIntegerOfWidth(64), target=cString)
class Substr(Instruction):
    """Extracts the substring of length *op3* from *op1* that starts at
    position *op2*."""

    def _codegen(self, cg):
        util.internal_error("string.substr not implemented")

@hlt.instruction("string.find", op1=cString, op2=cString, target=cIntegerOfWidth(64))
class Find(Instruction):
    """Searches *op2* in *op1*, returning the start index if it find it.
    Returns -1 if it does not find *op2* in *op1*."""

    def _codegen(self, cg):
        util.internal_error("string.find not implemented")

@hlt.instruction("string.cmp", op1=cString, op2=cString, target=cBool)
class Cmp(Instruction):
    """Compares *op1* with *op2* and returns True if their characters match.
    Returns False otherwise."""

    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::string_cmp", [self.op1(), self.op2()])
        boolean = cg.builder().icmp(llvm.core.IPRED_EQ, result, cg.llvmConstInt(0, 8))
        cg.llvmStoreInTarget(self, boolean)

@hlt.instruction("string.lt", op1=cString, op2=cString, target=cBool)
class Lt(Instruction):
    """Compares *op1* with *op2* and returns True if *op1* is
    lexicographically smaller than *op2*. Returns False otherwise."""

    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::string_cmp", [self.op1(), self.op2()])
        boolean = cg.builder().icmp(llvm.core.IPRED_SLT, result, cg.llvmConstInt(0, 8))
        cg.llvmStoreInTarget(self, boolean)

@hlt.instruction("string.decode", op1=cReferenceOf(cBytes), op2=_encoding, target=cString)
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
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::string_decode", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("string.encode", op1=cString, op2=_encoding, target=cReferenceOf(cBytes))
class Encode(Instruction):
    """Converts *op1* into bytes, encoding characters using the character set
    *op2*. Supported character sets are currently: ``ascii``, ``utf8``.

    If the any characters cannot be encoded with the given character set, they
    will be replaced with place-holders. If the character set given is not
    known, the instruction throws a ~~ValueError exception.

    Todo: We need to figure out how to support more character sets.
    """

    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::string_encode", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)


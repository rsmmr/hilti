# $Id$
"""
.. hlt:type:: enum

   The ``enum`` data type represents a selection of unique values, identified
   by labels. Enums must be defined in global space:

     enum Foo { Red, Green, Blue }

   The individual labels can then be used as global identifiers. In addition
   to the given labels, each ``enum`` type also implicitly defines one
   additional label called ''Undef''.

   If not explictly initialized, enums are set to ``Undef`` initially.

   TODO: Extend descriptions with the new semantics regarding storing
   undefined values.
"""

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

import string

def _makeVal(cg, undef, have_value, val):
    flags = cg.llvmConstInt( (1 if undef else 0) +  (2 if have_value else 0), 8)
    val = cg.llvmConstInt(val, 64)
    return llvm.core.Constant.struct([flags, val])

def _makeValLLVM(cg, undef, have_val, val):
    undef = cg.builder().zext(undef, llvm.core.Type.int(8))
    have_val = cg.builder().zext(have_val, llvm.core.Type.int(8))
    have_val = cg.builder().shl(have_val, cg.llvmConstInt(1, 32))
    flags = cg.builder().or_(undef, have_val)

    e = llvm.core.Constant.struct([cg.llvmConstInt(0, 8), cg.llvmConstInt(0, 64)])
    e = cg.llvmInsertValue(e, 0, flags)
    e = cg.llvmInsertValue(e, 1, val)

    return e

def _getUndef(cg, op):
    flags = cg.llvmExtractValue(op, cg.llvmConstInt(0, 32))
    bit = cg.builder().and_(flags, cg.llvmConstInt(1, 8))
    return cg.builder().trunc(bit, llvm.core.Type.int(1))

def _getHaveVal(cg, op):
    flags = cg.llvmExtractValue(op, cg.llvmConstInt(0, 32))
    bit = cg.builder().and_(flags, cg.llvmConstInt(2, 8))
    bit = cg.builder().lshr(bit, cg.llvmConstInt(1, 8))
    return cg.builder().trunc(bit, llvm.core.Type.int(1))

def _getVal(cg, op):
    return cg.llvmExtractValue(op, cg.llvmConstInt(1, 32))

@hlt.type("enum", 10, c="hlt_enum")
class Enum(type.ValueType, type.Constable):
    def __init__(self, labels):
        """The enum type.

        labels: list of string, or dict of string -> value - The labels (wo/
        any namespace, and excluding the ``Undef`` value) defined for the enum
        type. If a list is given, values are automatically assigned; otherwise
        the dictionary determines the mappings.
        """
        super(Enum, self).__init__()

        if isinstance(labels, list):
            i = 1
            self._labels = {}
            for t in labels:
                self._labels[t] = i
                i += 1

        else:
            self._labels = labels

    def labels(self):
        """Returns the enum labels with their corresponding integer values,
        excluding the Undef label.

        Returns: dictonary string -> int - The labels mappend to their values.
        """
        return self._labels

    def undef(self):
        """Returns a constant representing the undefined value.

        Returns: ~~Constant - The constant.
        """
        return constant.Constant("Undef", self)

    def isUndef(self, c):
        """Returns whether a constant represents the undefined value.

        Returns: bool - True if it is the undefined value.
        """
        return c.value() == "Undef"

    def llvmUndef(self, cg):
        """Returns the LLVM value representing the undefined value.

        Returns: llvm.core.Value - The value.
        """
        return _makeVal(cg, True, False, 0)

    ### Overridden from Type.

    def name(self):
        labels = ["%s = %d" % (l, v) for (l, v) in self._labels.items()]
        return "enum { %s }" % ", ".join(sorted(labels))

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        """An ``enum``'s type information keep additional information in the
        ``aux`` field: ``aux`` points to a concatenation of entries ``{
        uint64, hlt_string }``, one per label (excluding the ``Undef``
        label). The end of the array is marked by an string of null"""

        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::enum_to_string";
        typeinfo.to_int64 = "hlt::enum_to_int64";

        # Build ASCIIZ strings for the labels.
        aux = []
        zero = [cg.llvmConstInt(0, 8)]
        for (label, value) in sorted(self.labels().items(), key=lambda x: x[1]):
            label_glob = string._makeLLVMString(cg, label)

            aux += [llvm.core.Constant.struct([cg.llvmConstInt(value, 64), label_glob])]

        null = llvm.core.Constant.null(string._llvmStringTypePtr());
        aux += [llvm.core.Constant.struct([cg.llvmConstInt(0, 64), null])]

        name = cg.nameTypeInfo(self) + "_labels"
        const = llvm.core.Constant.array(aux[0].type, aux)
        glob = cg.llvmNewGlobalConst(name, const)
        glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY

        typeinfo.aux = glob

        return typeinfo

    def llvmType(self, cg):
        # Bit  0:    1 = Undefined value (i.e., one we don't know a label for); 0 otherwise.
        # Bit  1:    1 = No value set (only relevant if Byte 0 is set.); 0 otherwise.
        return llvm.core.Type.struct([llvm.core.Type.int(8), llvm.core.Type.int(64)])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """``enum`` variables are initialized with the ``Undef`` label."""
        return _makeVal(cg, True, False, 0)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        if not isinstance(const.value(), str):
            util.internal_error("enum label must be a Python string")

        if not const.value() in self._labels and not self.isUndef(const):
            vld.error(self, "undefined enum label %s" % const.value())

    def llvmConstant(self, cg, const):
        if self.isUndef(const):
            return self.llvmUndef(cg)

        return _makeVal(cg, False, False, self._labels[const.value()])

    def outputConstant(self, printer, const):
        for i in printer.currentModule().scope().IDs():
            if not isinstance(i, id.Type):
                continue

            if i.type() == self:
                namespace = i.name()

        printer.output(namespace + "::" + const.value())

    ### Overridden from None.
    def autodoc(self):
        for (l, v) in sorted(self._labels.items()):

            print "    .. hlt:global:: %s::%s %s" % (self._namespace, l, l)

            c = ["\n        No documentation for enum values yet."] # FIXME

            for line in c:
                print "        ", line

            print

@hlt.overload(Equal, op1=cEnum, op2=cSameTypeAsOp(1), target=cBool)
class Equal(Operator):
    """
    Returns True if *op1* equals *op2*. Note that two ``Undef`` value will
    match, no matter if they have different values set.
    """
    def _codegen(self, cg):
        op1 = cg.llvmOp(self.op1())
        op2 = cg.llvmOp(self.op2())

        # Compare undef flags.
        undef1 = _getUndef(cg, op1)
        undef2 = _getUndef(cg, op2)

        # Compare values.
        val1 = _getVal(cg, op1)
        val2 = _getVal(cg, op2)

        undef_or = cg.builder().or_(undef1, undef2)
        undef_and = cg.builder().and_(undef1, undef2)
        vals = cg.builder().icmp(llvm.core.IPRED_EQ, val1, val2)

        result = cg.builder().select(undef_or, undef_and, vals)

        cg.llvmStoreInTarget(self, result)

@hlt.instruction("enum.from_int", op1=cInteger, target=cEnum)
class FromInt(Instruction):
    """Converts the integer *op2* into an enum of the target's *type*. If the
    integer corresponds to a valid label (as set by explicit initialization),
    the results corresponds to that label. If integer does not correspond to
    any lable, the result will be of value ``Undef`` (yet internally, the
    integer value is retained.)
    """
    def _codegen(self, cg):
        labels = self.target().type().labels().items()

        block_cont = cg.llvmNewBlock("enum-cont")

        block_known = cg.llvmNewBlock("enum-known")
        cg.pushBuilder(block_known)
        cg.builder().branch(block_cont)
        cg.popBuilder()

        block_unknown = cg.llvmNewBlock("enum-unknown")
        cg.pushBuilder(block_unknown)
        cg.builder().branch(block_cont)
        cg.popBuilder()

        op = cg.llvmOp(self.op1())
        switch = cg.builder().switch(op, block_unknown)

        cases = []
        for (label, value) in labels:
            switch.add_case(cg.llvmConstInt(value, 64), block_known)

        cg.pushBuilder(block_cont)

        phi = cg.builder().phi(llvm.core.Type.int(1))
        phi.add_incoming(cg.llvmConstInt(0, 1), block_known)
        phi.add_incoming(cg.llvmConstInt(1, 1), block_unknown)

        result = _makeValLLVM(cg, phi, cg.llvmConstInt(1, 1), op)
        cg.llvmStoreInTarget(self, result)


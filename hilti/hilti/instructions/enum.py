# $Id$
"""
Enums
~~~~~

The ``enum`` data type represents a selection of unique values, identified by
labels. Enums must be defined in global space:

   enum Foo { Red, Green, Blue }

The individual labels can then be used as global identifiers. In addition to
the given labels, each ``enum`` type also implicitly defines one additional
label called ''Undef''.

If not explictly initialized, enums are set to
``Undef`` initially.

TODO: Extend descriptions with the new semantics regarding storing undefined values.
"""

import llvm.core

from hilti.constraints import *
from hilti.instructions.operators import *

import string

@hlt.type("enum", 10)
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
        return self._makeVal(cg, True, False, 0)

    def _makeVal(self, cg, undef, have_value, val):
        zero = cg.llvmConstInt(0, 1)
        one = cg.llvmConstInt(1, 1)
        val = cg.llvmConstInt(val, 64)
        return llvm.core.Constant.struct([one if undef else zero, one if have_value else zero, val])

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
        typeinfo.c_prototype = "hlt_enum"
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
        """An ``enum`` is mapped to an ``hlt_enum``."""
        # Byte  0:    1 = Undefined value (i.e., one we don't know a label for); 0 otherwise.
        # Byte  1:    1 = No value set (only relevant if Byte 0 is set.); 0 otherwise.
        # Bytes 2-10: Value.
        #
        # TODO: Check that LLVM's alignment for the 1-bit flags is indeed to use
        # two bytes.
        return llvm.core.Type.struct([llvm.core.Type.int(1), llvm.core.Type.int(1), llvm.core.Type.int(64)])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """``enum`` variables are initialized with the ``Undef`` label."""
        return self._makeVal(cg, True, False, 0)

    ### Overridden from Constable.

    def validateConstant(self, vld, const):
        if not isinstance(const.value(), str):
            util.internal_error("enum label must be a Python string")

        if not const.value() in self._labels and not self.isUndef(const):
            vld.error(self, "undefined enum label %s" % const.value())

    def llvmConstant(self, cg, const):
        if self.isUndef(const):
            return self.llvmUndef(cg)

        return self._makeVal(cg, False, False, self._labels[const.value()])

    def outputConstant(self, printer, const):
        for i in printer.currentModule().scope().IDs():
            if not isinstance(i, id.Type):
                continue

            if i.type() == self:
                namespace = i.name()

        printer.output(namespace + "::" + const.value())

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
        zero = cg.llvmConstInt(0, 32)
        undef1 = cg.llvmExtractValue(op1, zero)
        undef2 = cg.llvmExtractValue(op2, zero)

        # Compare values.
        two = cg.llvmConstInt(2, 32)
        val1 = cg.llvmExtractValue(op1, two)
        val2 = cg.llvmExtractValue(op2, two)

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

        result = llvm.core.Constant.struct([cg.llvmConstInt(0, 1), cg.llvmConstInt(1, 1), cg.llvmConstInt(0, 64)])
        result = cg.llvmInsertValue(result, 0, phi);
        result = cg.llvmInsertValue(result, 2, op);

        cg.llvmStoreInTarget(self, result)


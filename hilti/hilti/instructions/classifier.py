# $Id$
"""
.. hlt:type:: classifier

    A data type for performing packet classification, i.e., find the
    first match out of a collection of firewall-style rules.

    TODO.

    Note: for bytes, we do prefix-based matching.
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("classifier", 34, c="hlt_classifier *")
class Classifier(type.HeapType):
    """Type for a ``classifier``.

    rule: ~~TypeListable - The key type for all rules.
    value: ~~Type - The value type associated with a rule.
    """
    def __init__(self, rtype, vtype, location=None):
        super(Classifier, self).__init__(location)
        self._rtype = rtype
        self._vtype = vtype

    def ruleType(self):
        """Returns the type of the classifier's rules.

        Returns: ~~Type - The rule type.
        """
        return self._rtype

    def valueType(self):
        """Returns the type of the classifier's values associated with rules.

        Returns: ~~Type - The value type.
        """
        return self._vtype

    @staticmethod
    def llvmClassifierField(cg, data, len, bits = None):
        """Static method that converts bytes into the internal representation
        for classifier fields.

        data: llvm.core.Value - A pointer to ``i8`` with bytes for the
        classifier to match on. The *data* can become invalid after return
        from this method; if necessary, bytes will be copied. 

        len: llvm.core.Value - Number of bytes that ``data`` is pointing to.

        bits: llvm.core.Value - Optional number of *bits* valid in the byte
        sequence pointed to by *data*; the number must be equal or less than
        *len* times eight. If not given, all bytes will be matched on.

        Returns: llvm.core.Value - A pointer to a newly allocated struct of
        type ``hlt_classifier_field``.
        """
        ft = cg.llvmTypeInternal("__hlt_classifier_field")
        size = cg.llvmSizeOf(ft)
        size = cg.builder().add(size, len)
        field = cg.llvmMallocBytes(size, "hlt_classifier_field", cast_to=llvm.core.Type.pointer(ft))

        if bits == None:
            bits = cg.builder().mul(len, cg.llvmConstInt(8, 64))

        # Initialize the static attributes of the field.
        struct = llvm.core.Constant.null(ft)
        struct = cg.llvmInsertValue(struct, 0, len)
        struct = cg.llvmInsertValue(struct, 1, bits)
        cg.llvmAssign(struct, field)

        # Copy the data bytes into the field.
        if data != None:
            data = cg.builder().bitcast(data, cg.llvmTypeGenericPointer())
            i8p = cg.llvmTypeGenericPointer()
            dst = cg.builder().gep(field, [cg.llvmGEPIdx(0), cg.llvmGEPIdx(2)])
            dst = cg.builder().bitcast(dst, i8p)
            cg.llvmCallIntrinsic(llvm.core.INTR_MEMCPY, [i8p, i8p, len.type], [dst, data, len, cg.llvmConstInt(1, 32), cg.llvmConstInt(0, 1)])

        return cg.builder().bitcast(field, cg.llvmTypeGenericPointer())

    @staticmethod
    def llvmClassifierFieldMatchAll(cg):
        null = llvm.core.Constant.null(cg.llvmTypeGenericPointer())
        return Classifier.llvmClassifierField(cg, None, cg.llvmConstInt(0, 64))

    ### Overridden from Type.

    def _validate(self, vld):
        super(Classifier, self)._validate(vld)

        if not self._rtype and not self._vtype:
            # Wildcards for passing into libhilti.
            return

        if not isinstance(self._rtype, type.TypeListable):
            vld.error(self, "invalid rule type for classifier")

        for t in self._rtype.typeList():
            if not isinstance(t, type.Classifiable) and \
               not (isinstance(t, type.Reference) and isinstance(t.refType(), type.Classifiable)):
                vld.error(self, "invalid field type in rule type")

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

@hlt.constraint("*value-type*")
def _cValType(ty, op, i):
	vtype = i.op1().type().refType().valueType()
	return (op.canCoerceTo(ty), "must be of type %s but is %s" % (vtype, ty))

@hlt.constraint("*key-type*")
def _cMatchingVal(ty, op, i):
    rtype = i.op1().type().refType().ruleType()
    assert isinstance(rtype, type.TypeListable)

    if isinstance(ty, type.Reference):
        ty = ty.refType()

    if not isinstance(ty, type.TypeListable):
        return (False, "invalid type for key value")

    if len(rtype.typeList()) != len(ty.typeList()):
        return (False, "invalid types for key value")

    for (r, k) in zip(rtype.typeList(), ty.typeList()):
        if not isinstance(r, type.Classifiable) and \
           not (isinstance(r, type.Reference) and isinstance(r.refType(), type.Classifiable)):
            assert False

        if not k.canCoerceTo(r):
            return (False, "incompatible type in key value")

    return (True, None)

@hlt.constraint("*rule*")
def _cRule(ty, op, i):
    rtype = i.op1().type().refType().ruleType()
    assert isinstance(rtype, type.TypeListable)

    if not isinstance(ty, type.Reference):
        return (False, "invalid type for rule")

    if not isinstance(ty.refType(), type.TypeListable):
        return (False, "invalid type for rule")

    if len(rtype.typeList()) != len(ty.refType().typeList()):
        return (False, "invalid types for rule fields")

    if not rtype == ty.refType():
        return (False, "rule type does not match declaration")

    return (True, None)

@hlt.constraint("*value-type*")
def _cValue(ty, op, i):
    vtype = i.op1().type().refType().valueType()
    return (ty.canCoerceTo(vtype), "incompatible type for rule value")

def _llvmFields(cg, rtype, vtype, val):

    ftypes = rtype.typeList()
    vtypes = vtype.typeList()

    atype = llvm.core.Type.array(cg.llvmTypeGenericPointer(), len(ftypes))
    fields = cg.llvmMalloc(atype, "classifier field array")

    # Convert the fields into the internal hlt_classifier_field representation.

    empty = Classifier.llvmClassifierFieldMatchAll(cg)

    for i in range(len(ftypes)):

        # Helper function to convert a given LLVM value to a classifier field.
        def _toField(cg, val):
            ty = ftypes[i]

            if isinstance(ty, type.Reference):
                ty = ty.refType()

            return ty.llvmToField(cg, vtypes[i], val)

        field = vtype.llvmGetField(cg, val, i, default=empty, func=_toField)
        idx = cg.llvmGEPIdx(i)
        addr = cg.builder().gep(fields, [cg.llvmGEPIdx(0), idx])
        cg.llvmAssign(field, addr)

    fields = cg.builder().bitcast(fields, cg.llvmTypeGenericPointer())
    return operand.LLVM(fields, type.Any())

@hlt.overload(New, op1=cType(cClassifier), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new classifier of type *op1*. Once allocated, rules can be
    added with ``classifier.add``.
    """
    def _codegen(self, cg):
        rtype = operand.Type(self.op1().value().ruleType())
        vtype = operand.Type(self.op1().value().valueType())

        num_fields = constant.Constant(len(rtype.value().typeList()), type.Integer(64))
        num_fields = operand.Constant(num_fields)

        result = cg.llvmCallC("hlt::classifier_new", [num_fields, rtype, vtype])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("classifier.add", op1=cReferenceOf(cClassifier), op2=cIsTuple([_cRule, cInteger]), op3=_cValue)
class Add(Instruction):
    """Adds a rule to classifier *op1*. The first element of *op2* is the
    rule's fields, and the second element is the rule's match priority. If
    multiple rules match with a later lookup, the rule with the highest
    priority will win. *op3* is the value that will be associated with the
    rule.

    This instruction must not be called anymore after ``classifier.compile``
    has been executed.
    """
    def _codegen(self, cg):
        op2 = cg.llvmOp(self.op2())
        rule = self.op2().type().llvmIndex(cg, op2, 0)
        prio = self.op2().type().llvmIndex(cg, op2, 1)

        rtype = self.op1().type().refType().ruleType()
        vtype = self.op2().type().types()[0].refType()
        fields = _llvmFields(cg, rtype, vtype, rule)

        cg.llvmCallC("hlt::classifier_add", [self.op1(), fields, operand.LLVM(prio, self.op2().type().types()[1]), self.op3()])

@hlt.instruction("classifier.compile", op1=cReferenceOf(cClassifier))
class Compile(Instruction):
    """Freezes the classifier *op1* and prepares it for subsequent lookups."""
    def _codegen(self, cg):
        cg.llvmCallC("hlt::classifier_compile", [self.op1()])

@hlt.instruction("classifier.matches", op1=cReferenceOf(cClassifier), op2=_cMatchingVal, target=cBool)
class Matches(Instruction):
    """Returns True if *op2* matches any rule in classifier *op1*.

    This instruction must only be used after the classifier has been fixed
    with ``classifier.compile``.
    """
    def _codegen(self, cg):
        op2 = self.op2()
        rtype = self.op1().type().refType().ruleType()
        vtype = self.op2().type()

        if isinstance(vtype, type.Tuple):
            # Turn the tuple into a struct.
            op2.coerceTo(cg, type.Reference(rtype))

        fields = _llvmFields(cg, rtype, rtype, cg.llvmOp(op2))

        result = cg.llvmCallC("hlt::classifier_matches", [self.op1(), fields])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("classifier.get", op1=cReferenceOf(cClassifier), op2=_cMatchingVal, target=_cValType)
class Get(Instruction):
    """Returns the value associated with the highest-priority rule matching
    *op2* in classifier *op1*. Throws an IndexError exception if no match is
    found. If multiple rules of the same priority match, it is undefined which
    one will be selected.

    This instruction must only be used after the classifier has been fixed
    with ``classifier.compile``.
    """
    def _codegen(self, cg):
        op2 = self.op2()
        rtype = self.op1().type().refType().ruleType()
        vtype = self.op2().type()

        if isinstance(vtype, type.Tuple):
            # Turn the tuple into a struct.
            op2.coerceTo(cg, type.Reference(rtype))

        fields = _llvmFields(cg, rtype, rtype, cg.llvmOp(op2))

        voidp = cg.llvmCallC("hlt::classifier_get", [self.op1(), fields])
        casted = cg.builder().bitcast(voidp, llvm.core.Type.pointer(cg.llvmType(self.target().type())))
        cg.llvmStoreInTarget(self, cg.builder().load(casted))



# $Id$
"""
Structures
~~~~~~~~~~

The ``struct`` data type groups a set of heterogenous, named fields. Each
instance tracks which fields have already been set. Fields may optionally
provide defaul values that are returned when it's read but hasn't been set
yet.
"""

builtin_id = id

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("struct", 7)
class Struct(type.HeapType):
    """Type for ``struct``.

    fields: list of (~~ID, ~~Operand) - The fields of the struct, given as
    tuples of an ID and an optional default value; if a field does not have a
    default value, use None as the operand.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, fields, location=None):
        super(Struct, self).__init__(location=location)
        self._ids = fields

    def fields(self):
        """Returns the struct's fields.

        Returns: list of (~~ID, ~~Operand) - The struct's fields, given as
        tuples of an ID and an optional default value.
        """
        return self._ids

    ### Overridden from Type.

    _done = None

    def name(self):
        # We need to avoid infinite recursions here. This kind of a hack
        # though ...
        first = False
        if not Struct._done:
            first = True
            Struct._done = {}

        idx = builtin_id(self)

        try:
            return Struct._done[idx]
        except KeyError:
            pass

        Struct._done[idx] = "<recursive struct>"
        name = "struct { %s }" % ", ".join(["%s %s" % (id.type(), id.name()) for (id, op) in self._ids])
        Struct._done[idx] = name

        if first:
            Struct._done = None

        return name

    def _resolve(self, resolver):
        for (i, op) in self._ids:
            i.resolve(resolver)
            if op:
                op.resolve(resolver)

        return self

    def _validate(self, vld):
        for (i, op) in self._ids:
            i.validate(vld);
            if op:
                op.validate(vld);

    def output(self, printer):
        printer.output("struct {", nl=True)
        printer.push()
        first = True
        for (id, op) in self._ids:
            if not first:
                printer.output(",", nl=True)
            id.output(printer)
            if op:
                printer.output(" &default=")
                op.output(printer)
            first = False

        printer.output("", nl=True)
        printer.pop()
        printer.output("}")

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        """Type information for a ``struct`` includes the fields' offsets in
        the ``aux`` entry as a concatenation of pairs (ASCIIZ*, offset), where
        ASCIIZ is a field's name, and offset its offset in the value.
        """
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::struct_to_string";
        typeinfo.args = [id.type() for (id, op) in self.fields()]

        zero = cg.llvmGEPIdx(0)
        null = llvm.core.Constant.null(self.llvmType(cg))

        array = []
        for i in range(len(self.fields())):

            # Make the field name.
            str = cg.llvmNewGlobalStringConst(cg.nameNewGlobal("struct-field"), self.fields()[i][0].name())

            # Calculate the offset.

                # We skip the bitmask here.
            idx = cg.llvmGEPIdx(i + 1)

                # This is a pretty awful hack but I can't find a nicer way to
                # calculate the offsets as *constants*, and this hack is actually also
                # used by LLVM internaly to do sizeof() for constants so it can't be
                # totally disgusting. :-)
            offset = null.gep([zero, idx]).ptrtoint(llvm.core.Type.int(16))
            struct = llvm.core.Constant.struct([str, offset])

            array += [llvm.core.Constant.struct([str, offset])]

        if array:

            name = cg.nameTypeInfo(self) + "-fields"

            const = llvm.core.Constant.array(array[0].type, array)
            glob = cg.llvmNewGlobalConst(name, const)
            glob.linkage = llvm.core.LINKAGE_LINKONCE_ANY

            typeinfo.aux = glob

        else:
            typeinfo.aux = None

        return typeinfo

    def llvmType(self, cg):
        """A ``struct` is passed as a pointer to an eqivalent C struct; the
        fields' types are converted recursively to C using standard rules."""

        th = llvm.core.TypeHandle.new(llvm.core.Type.opaque())

        assert len(self.fields()) <= 32
        fields = [llvm.core.Type.int(32)] + [cg.llvmType(id.type()) for (id, default) in self.fields()]
        ty = llvm.core.Type.pointer(llvm.core.Type.struct(fields))
        th.type.refine(ty)
        return ty

@hlt.constraint("string")
def _fieldName(ty, op, i):
    if not op or not isinstance(op, operand.Operand):
        return (False, "index must be an operand")

    c = op.constant()

    if not c:
        return (False, "index must be constant")

    if not isinstance(ty, type.String):
        return (False, "index must be string")

    for (id, default) in i.op1().type().refType().fields():
        if c.value() == id.name():
            return (True, "")

    return (False, "%s is not a field name in %s" % (op.value(), i.op1().type().refType()))

@hlt.constraint("any")
def _fieldType(ty, op, i):

    c = i.op2().constant()

    for (id, default) in i.op1().type().refType().fields():
        if c.value() == id.name():
            if op.canCoerceTo(id.type()):
                return (True, "")
            else:
                return (False, "type must be %s" % id.type())

    assert False

@hlt.overload(New, op1=cType(cStruct), target=cReferenceOfOp(1))
class New(Operator):
    """Allocates a new instance of the structure given as *op1*. All fields
    will initially be unset.
    """
    def _codegen(self, cg):
        # Allocate memory for struct.
        if isinstance(self.op1(), operand.Type):
            structt = self.op1().value()
        else:
            structt = self.op1().value().type()

        s = cg.llvmMalloc(structt.llvmType(cg).pointee)

        # Initialize fields
        zero = cg.llvmGEPIdx(0)
        mask = 0

        fields = structt.fields()
        for j in range(len(fields)):
            (id, default) = fields[j]
            if default:
                # Initialize with default.
                mask |= (1 << j)
                index = cg.llvmGEPIdx(j + 1)
                addr = cg.builder().gep(s, [zero, index])
                cg.llvmAssign(cg.llvmOp(default, id.type()), addr)
            else:
                # Leave untouched. As we keep the bitmask of which fields are
                # set,  we will never access it.
                pass

        # Set mask.
        addr = cg.builder().gep(s, [zero, zero])
        cg.llvmAssign(cg.llvmConstInt(mask, 32), addr)

        cg.llvmStoreInTarget(self, s)

@hlt.instruction("struct.get", op1=cReferenceOf(cStruct), op2=_fieldName, target=_fieldType)
class Get(Instruction):
    """Returns the field named *op2* in the struct referenced by *op1*. The
    field name must be a constant, and the type of the target must match the
    field's type. If a field is requested that has not been set, its default
    value is returned if it has any defined. If it has not, an
    ``UndefinedValue`` exception is raised.
    """
    def _codegen(self, cg):
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Check whether field is valid.
        zero = cg.llvmGEPIdx(0)

        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)

        bit = cg.llvmConstInt(1<<idx, 32)
        isset = cg.builder().and_(bit, mask)

        block_ok = cg.llvmNewBlock("ok")
        block_exc = cg.llvmNewBlock("exc")

        notzero = cg.builder().icmp(llvm.core.IPRED_NE, isset, cg.llvmConstInt(0, 32))
        cg.builder().cbranch(notzero, block_ok, block_exc)

        cg.pushBuilder(block_exc)
        cg.llvmRaiseExceptionByName("hlt_exception_undefined_value", self.location())
        cg.popBuilder()

        cg.pushBuilder(block_ok)

        # Load field.
        index = cg.llvmGEPIdx(idx + 1)
        addr = cg.builder().gep(s, [zero, index])
        cg.llvmStoreInTarget(self, cg.builder().load(addr))

@hlt.instruction("struct.set", op1=cReferenceOf(cStruct), op2=_fieldName, op3=_fieldType)
class Set(Instruction):
    """
    Sets the field named *op2* in the struct referenced by *op1* to the value
    *op3*. The type of the *op3* must match the type of the field.
    """
    def _codegen(self, cg):
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Set mask bit.
        zero = cg.llvmGEPIdx(0)
        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)
        bit = cg.llvmConstInt(1<<idx, 32)
        mask = cg.builder().or_(bit, mask)
        cg.llvmAssign(mask, addr)

        index = cg.llvmGEPIdx(idx + 1)
        addr = cg.builder().gep(s, [zero, index])
        cg.llvmAssign(cg.llvmOp(self.op3(), ftype), addr)


@hlt.instruction("struct.unset", op1=cReferenceOf(cStruct), op2=_fieldName)
class Unset(Instruction):
    """Unsets the field named *op2* in the struct referenced by *op1*. An
    unset field appears just as if it had never been assigned an value; in
    particular, it will be reset to its default value if has been one assigned.
    """
    def _codegen(self, cg):
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Clear mask bit.
        zero = cg.llvmGEPIdx(0)
        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)
        bit = cg.llvmConstInt(~(1<<idx), 32)
        mask = cg.builder().and_(bit, mask)
        cg.llvmAssign(mask, addr)

@hlt.instruction("struct.is_set", op1=cReferenceOf(cStruct), op2=_fieldName, target=cBool)
class IsSet(Instruction):
    """Returns *True* if the field named *op2* has been set to a value, and
    *False otherwise. If the instruction returns *True*, a subsequent call to
    ~~Get will not raise an exception.
    """
    def _codegen(self, cg):
        (idx, ftype) = _getIndex(self)
        assert idx >= 0

        s = cg.llvmOp(self.op1())

        # Check mask.
        zero = cg.llvmGEPIdx(0)
        addr = cg.builder().gep(s, [zero, zero])
        mask = cg.builder().load(addr)

        bit = cg.llvmConstInt(1<<idx, 32)
        isset = cg.builder().and_(bit, mask)

        notzero = cg.builder().icmp(llvm.core.IPRED_NE, isset, cg.llvmConstInt(0, 32))
        cg.llvmStoreInTarget(self, notzero)

def _getIndex(instr):

    fields = instr.op1().type().refType().fields()

    for i in range(len(fields)):
        (id, default) = fields[i]
        c = instr.op2().constant()
        if id.name() == c.value():
            return (i, id.type())

    return (-1, None)

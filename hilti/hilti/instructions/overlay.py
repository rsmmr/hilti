# $Id$
#
# Overlays keep two kinds of state:
#
#   - The starting position corresponding to offset 0. This is set by the
#     "attach" instruction.
#
#   - For each field that is a dependency for determining another field's
#     starting offset, we cache its end position once we have calculated it
#     once.
"""
Todo.
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

@hlt.type("overlay", 14)
class Overlay(type.ValueType):
    """Type for ``overlay``."""

    class Field:
        def __init__(self, name, start, type, fmt, arg = None, location=None):
            """
            Defines one field of the overlay.

            name: string - The name of the field.

            start: integer or string - If an integer, the field is assumed to
            be the offset in bytes from the start of the overlay where the
            field's data starts. If it's a string, the string must be the name
            of an another field and the added field is then assumed to start
            right after that one.

            type: ~~Type - The type of the field.

            fmt: ~~Operand - An operand refering to a ``Hilti::Packed`` label
            defining the format used for unpacked the field.
            """
            self._name = name
            self._start = start
            self._type = type
            self._fmt = fmt
            self._arg = arg

            self._deps = []
            self._offset = -1
            self._idx = -1

            self._location = location

        def location(self):
            """Returns location information for the field.

            Returns:: ~~Location - The field's location."""
            return self._location

        def name(self):
            """Returns the name of the field.

            Returns: string - The name of the field.
            """
            return self._name

        def type(self):
            """Returns the type of the field.

            Returns: HiltiType - The type of the field.
            """
            return self._type

        def start(self):
            """Returns the field's start.

            start: integer or string - If an integer, it is the offset in
            bytes from the start of the overlay. If it's a string, the string
            is the name of an another field and the field then assumed
            starts right after that one.
            """
            return self._start

        def fmt(self):
            """Returns the format of the field.

            Returns: integer - The value corresponds to the internal enum
            value of the ``Hilti::Packed`` label defining the format used for
            unpacking the field.
            """
            return self._fmt

        def arg(self):
            """Returns the optional argument of the field.

            Returns: operand.Operand or None - The operand or None if none.
            """
            return self._arg

        def offset(self):
            """Returns the constant offset for the field.

            Returns: integer - The offset in bytes from the start of the overlay.
            If the name defines a field with a non-constant offset, -1 returned.
            """
            return self._offset

        def dependants(self):
            """Returns all other fields the field depends on for its
            starting position.

            Returns: list of string - The names of other fields which are
            required to calculate the starting offset for this field. The
            list will be sorted in the order the fields were added to the
            overlay by ~~addField. If the named field starts at a constant
            offset and does not depend on any other fields, an empty list
            is returned.
            """
            return self._deps

        def depIndex(self):
            """Returns an index unique across all fields that are directly
            determining the starting position of another field with a
            non-constant offset.  All of these fields are sequentially
            numbered, starting with one.

            Returns: integer - The index of this field, or -1 if the field
            does not determine another's offset directly.
            """
            return self._idx

        def __str__(self):
            return self._name

        ### These are equivalent to the corresponding methods from ast.Node

        def resolve(self, resolver):
            self._type = self._type.resolve(resolver)
            self._fmt.resolve(resolver)

        def validate(self, vld):
            self._type.validate(vld)

            packed = vld.currentModule().scope().lookup("Hilti::Packed")
            assert packed

            if self._fmt.type() != packed.type():
                vld.error(self, "%s is not of type Hilti::Packed, but is %s" % (self._fmt, self._fmt.type()))

        def output(self, printer):
            if isinstance(self._start, str):
                pos = "after %s" % self._start
            else:
                pos = "at %d" % self._start

            printer.output("%s: " % self._name)
            printer.printType(self._type)
            printer.output(" %s unpack with " % pos)
            self._fmt.output(printer)
            if self._arg:
                printer.output(" ")
                self._arg.output(printer)

    def __init__(self, location=None):
        super(Overlay, self).__init__(location=location)
        self._fields = {}
        self._deps = {}
        self._idxcnt = 0

    def addField(self, field):
        """Adds a field to an overlay. If the *field*'s *start* attribute
        specifies another field's name, that one must already have been added
        earlier.

        field: ~~Field - The field to be added.
        """

        if field._name in self._fields:
            raise HiltiType.ParameterMismatch(args[0], "field %s already defined" % field._name)

        if isinstance(field._start, str):
            # Field depends on another one.
            if not field._start in self._fields:
                raise HiltiType.ParameterMismatch(args[0], "field %s not yet defined" % field._start)

            field._deps = self._fields[field._start]._deps + [field._start]

            start = self._fields[field._start]
            if start._idx == -1:
                self._idxcnt += 1
                start._idx = self._idxcnt

        else:
            # Field has a constant offset.
            field._offset = field._start

        self._fields[field._name] = field

    def fields(self):
        """Returns a list of all defined fields.

        Returns: list of ~~Field - The fields.
        """
        return self._fields.values()

    def field(self, name):
        """Returns the named field.

        name: string - Name of the field requested.

        Returns: ~~Field - The field, or None if there's no such field.
        """
        return self._fields.get(name, None)

    def numDependencies(self):
        """Returns the number of fields that other fields directly depend on
        for their starting position. This number is guaranteed to be not
        higher than the largest ~depIndex in this overlay.

        Returns: integer - The number of fields.
        """
        return self._idxcnt

    ### Overridden from Type.

    def name(self):
        return "overlay { %s }" % ", ".join([str(f) for f in self._fields.values()])

    def _resolve(self, resolver):
        for f in self._fields.values():
            f.resolve(resolver)

        return self

    def _validate(self, vld):
        for f in self._fields.values():
            f.validate(vld)

    def output(self, printer):
        printer.output("overlay {", nl=True)
        printer.push()
        first = True
        for f in self._fields.values():
            if not first:
                printer.output(", ", nl=True)
            f.output(printer)
            first = False
        printer.output("", nl=True)
        printer.pop()
        printer.output("}", nl=True)

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_overlay";
        return typeinfo

    def llvmType(self, cg):
        """An ``overlay`` is mapped to a ``hlt_overlay``."""
        itype = cg.llvmType(type.IteratorBytes())
        return llvm.core.Type.array(itype, self._arraySize())

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """An ``overlay`` is intially unattached."""
        itype = cg.llvmType(type.IteratorBytes())
        end = cg.llvmConstDefaultValue(type.IteratorBytes())
        return llvm.core.Constant.array(itype, [end] * self._arraySize())

    # Private.
    def _arraySize(self):
        return 1 + self.numDependencies()

@hlt.constraint("string")
def _fieldName(ty, op, i):
    if not op or not isinstance(op, operand.Constant):
        return (False, "field name must be constant")

    if not isinstance(ty, type.String):
        return (False, "field name must be a string")

    return (i.op1().type().field(op.value().value()) != None,
            "%s is not a field name in %s" % (op.value().value(), i.op1().type()))

@hlt.constraint("any")
def _fieldType(ty, op, i):
    ft = i.op1().type().field(i.op2().value().value()).type()
    return (ty == ft, "target type must be %s" % ft)

@hlt.instruction("overlay.attach", op1=cOverlay, op2=cIteratorBytes)
class Attach(Instruction):
    """Associates the overlay in *op1* with a bytes object, aligning the
    overlays offset 0 with the position specified by *op2*. The *attach*
    operation must be performed before any other ``overlay`` instruction is
    used on *op1*.
    """
    def _codegen(self, cg):
        overlayt = self.op1().type()

        # Save the starting offset.
        ov = cg.llvmOp(self.op1())
        iter = cg.llvmOp(self.op2())
        ov = cg.llvmInsertValue(ov, 0, iter)

        # Clear the cache
        end = cg.llvmConstDefaultValue(type.IteratorBytes())
        for j in range(1, overlayt._arraySize() + 1):
            ov = cg.llvmInsertValue(ov, j, end)

        # Need to rewrite back into the original value.
        self.op1().llvmStore(cg, ov)

@hlt.instruction("overlay.get", op1=cOverlay, op2=_fieldName, target=_fieldType)
class Get(Instruction):
    """Unpacks the field named *op2* from the bytes object attached to the
    overlay *op1*. The field name must be a constant, and the type of the
    target must match the field's type.

    The instruction throws an OverlayNotAttached exception if
    ``overlay.attach`` has not been executed for *op1* yet.
    """
    def _codegen(self, cg):
        overlayt = self.op1().type()
        ov = cg.llvmOp(self.op1())
        offset0 = cg.llvmExtractValue(ov, 0)

        # Generate the unpacking code for one field, assuming all dependencies are
        # already resolved.
        def _emitOne(ov, field):
            offset = field.offset()

            if offset == 0:
                # Starts right at the beginning.
                begin = offset0

            elif offset > 0:
                # Static offset. We can calculate the starting position directly.
                arg1 = operand.LLVM(offset0, type.IteratorBytes())
                arg2 = operand.Constant(constant.Constant(offset, type.Integer(32)))
                begin = cg.llvmCallC("hlt::bytes_pos_incr_by", [arg1, arg2])

            else:
                # Offset is not static but our immediate dependency must have
                # already been calculated at this point so we take it's end
                # position out of the cache.
                deps = field.dependants()
                assert deps
                dep = overlayt.field(deps[-1]).depIndex()
                assert dep > 0
                begin = cg.llvmExtractValue(ov, dep)

            if field.arg():
                arg = cg.llvmOp(field.arg())
                arg = operand.LLVM(arg, field.arg().type())
            else:
                arg = None

            (val, end) = cg.llvmUnpack(field.type(), begin, None, field.fmt(), arg)

            if field.depIndex() >= 0:
                ov = cg.llvmInsertValue(ov, field.depIndex(), end)

            return (ov, val)

        # Generate code for all depedencies.
        def _makeDep(ov, field):

            if not field.dependants():
                return ov

            # Need a tmp or a PHI node. LLVM's mem2reg should optimize the tmp
            # away so this is easier.
            # FIXME: Check that mem2reg does indeed.
            new_ov = cg.llvmAlloca(ov.type)
            dep = overlayt.field(field.dependants()[-1])
            block_cont = cg.llvmNewBlock("have-%s" % dep.name)
            block_cached = cg.llvmNewBlock("cached-%s" % dep.name)
            block_notcached = cg.llvmNewBlock("not-cached-%s" % dep.name)

            _isNull(cg, ov, dep.depIndex(), block_notcached, block_cont)

            cg.pushBuilder(block_notcached)
            ov = _makeDep(ov, dep)
            cg.builder().store(ov, new_ov)
            cg.builder().branch(block_cont)
            cg.popBuilder()

            cg.pushBuilder(block_cached)
            cg.builder().store(ov, new_ov)
            cg.builder().branch(block_cont)
            cg.popBuilder()

            cg.pushBuilder(block_cont)
            ov = cg.builder().load(new_ov)
            (ov, val) = _emitOne(ov, dep)
            return ov

        # Make sure we're attached.
        block_attached = cg.llvmNewBlock("attached")
        block_notattached = cg.llvmNewBlock("not-attached")

        cg.pushBuilder(block_notattached)
        cg.llvmRaiseExceptionByName("hlt_exception_overlay_not_attached", self.location())
        cg.popBuilder()

        _isNull(cg, ov, 0, block_notattached, block_attached)

        cg.pushBuilder(block_attached) # Leave on stack.

        field = overlayt.field(self.op2().value().value())
        ov = _makeDep(ov, field)
        (ov, val) = _emitOne(ov, field)
        cg.llvmStoreInTarget(self, val)


def _isNull(cg, ov, idx, block_yes, block_no):
    # FIXME: I guess we should do this check somewhere in the bytes code as
    # technically we can't know here what the internal representation of an
    # iterator is ...
    val = cg.llvmExtractValue(ov, idx)
    chunk = cg.llvmExtractValue(val, 0)
    cmp = cg.builder().icmp(llvm.core.IPRED_EQ, chunk, llvm.core.Constant.null(cg.llvmTypeGenericPointer()))
    cg.builder().cbranch(cmp, block_yes, block_no)

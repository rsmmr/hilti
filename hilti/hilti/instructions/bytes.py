# $Id$
"""
.. hlt:type:: bytes

   ``bytes`` is a data type for storing sequences of raw bytes. It is
   optimized for storing and operating on large amounts of unstructured data.
   In particular, it provides efficient subsequence and append operations.
   Bytes are forward-iterable.
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

def llvmEnd(cg):
    """Returns an ``llvm.core.Value`` that marks the end of a (any) bytes
    object.

    Note: The value returned must correspond to what ``bytes.c`` expects as
    the end-of-bytes marker, obviously."""
    return llvm.core.Constant.struct([llvm.core.Constant.null(cg.llvmTypeGenericPointer())] * 2)

@hlt.type(None, 100, doc="iterator<:hlt:type:`bytes`>", c="hlt_bytes_pos")
class IteratorBytes(type.Iterator):
    """Type for iterating over ``bytes``.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, location=None):
        super(IteratorBytes, self).__init__(type.Bytes(), location=location)

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        return cg.TypeInfo(self)

    def llvmType(self, cg):
        return cg.llvmTypeInternal("__hlt_bytes_pos")

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """A ``bytes`` iterator is initially set to the end of (any) ``bytes``
        object."""
        return llvmEnd(cg)

    ### Overridden from Iterator.

    def derefType(self):
        return type.Integer(8)

@hlt.type("bytes", 9, c="hlt_bytes *")
class Bytes(type.HeapType, type.Constructable, type.Iterable, type.Unpackable):
    """Type for ``bytes``.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, location=None):
        super(Bytes, self).__init__(location=location)

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.to_string = "hlt::bytes_to_string"
        typeinfo.hash = "hlt::bytes_hash"
        typeinfo.equal = "hlt::bytes_equal"
        return typeinfo

    ### Overridden from Constructable.

    def validateCtor(self, cg, val):
        return isinstance(val, str)

    def llvmCtor(self, cg, val):
        """The constructor for ``bytes`` resembles the one for strings:
        ``b"abcdef"`` creates a new ``bytes`` object and initialized it with
        six bytes. Note that such ``bytes`` instances are *not* constants but
        can be modified later on."""
        # We create two globals:
        #
        # (1) one for storing the raw characters themselves.
        # (2) one for storing the bytes objects (which will point to (1))
        #
        # Note that (2) needs to be dynamically intialized via an LLVM ctor.

        size = len(val)
        array = [llvm.core.Constant.int(llvm.core.Type.int(8), ord(c)) for c in val]
        array = llvm.core.Constant.array(llvm.core.Type.int(8), array)

        data = cg.llvmNewGlobalConst(cg.nameNewGlobal("bytes"), array)
        datac = cg.builder().bitcast(data, self.llvmType(cg))

        exception = cg.llvmAlloca(cg.llvmTypeExceptionPtr())
        ctx = cg.llvmCurrentExecutionContextPtr()
        newobj = cg.llvmCallCInternal("hlt_bytes_new_from_data", [datac, cg.llvmConstInt(size, 64), exception, ctx])

        return newobj

    def outputCtor(self, printer, val):
        printer.output('b"%s"' % util.unicode_escape(val))

    ### Overriden from Iterable.

    def iterType(self):
        return IteratorBytes()

    ### Overriden from Unpackable.

    def formats(self, mod):
        """The ``Skip*`` variants do always return an empty ``bytes`` object
        but still advance the unpacking iterator in the same way as the
        non-skip version. This can be used if the data itself is not
        uninteresting but it's end position is needed to then unpack any
        subsequent data.
        """

        packed = mod.scope().lookup("Hilti::Packed")

        if not packed:
            util.internal_error("type Hilti::Packed is not defined")

        packed = packed.type()

        return [
            ("BytesRunLength", packed, False,
              "A series of bytes preceded by an uint indicating its length."),

            ("BytesFixed", type.Integer(64), False,
              "A series of bytes of fixed length specified by an additional integer argument"),

            ("BytesDelim", type.Reference(type.Bytes()), False,
              "A series of bytes delimited by a final byte-sequence specified by an additional argument."),

            ("SkipBytesRunLength", packed, False,
              "Like BytesRunLength, but does not return unpacked value."),

            ("SkipBytesFixed", type.Integer(64), False,
              "Like BytesFixed, but does not return unpacked value."),

            ("SkipBytesDelim", type.Reference(type.Bytes()), False,
              "Like BytesDelim, but does not return unpacked value."),
            ]

    def llvmUnpack(self, cg, begin, end, fmt, arg):
        bytes = cg.llvmAlloca(self)
        iter = cg.llvmAlloca(type.IteratorBytes())
        exception = cg.llvmFrameExceptionAddr()
        ctx = cg.llvmCurrentExecutionContextPtr()

        def unpackFixed(n, skip, begin):
            def _unpackFixed(case):
                begin_op = operand.LLVM(begin, type.IteratorBytes())
                next = cg.llvmCallC("hlt::bytes_pos_incr_by", [begin_op, n])
                next_op = operand.LLVM(next, type.IteratorBytes())
                end = operand.LLVM(llvmEnd(cg), type.IteratorBytes())

                is_end = cg.llvmCallC("hlt::bytes_pos_eq", [next_op, end])

                block_end = cg.llvmNewBlock("unpack-at-end")
                block_done = cg.llvmNewBlock("unpack-got-it")
                block_insufficient = cg.llvmNewBlock("unpack-out-of-input")

                cg.builder().cbranch(is_end, block_end, block_done)

                # When we've reached the end, we must check whether we still
                # got enough bytes or not.
                cg.pushBuilder(block_end)
                len = cg.llvmCallC("hlt::bytes_pos_diff", [begin_op, end])
                len = cg.builder().zext(len, llvm.core.Type.int(64))
                enough = cg.builder().icmp(llvm.core.IPRED_ULT, len, cg.llvmOp(n))
                cg.builder().cbranch(enough, block_insufficient, block_done)
                cg.popBuilder()

                builder = cg.pushBuilder(block_insufficient)
                cg.llvmRaiseExceptionByName("hlt_exception_would_block", n.location())
                cg.popBuilder()

                builder = cg.pushBuilder(block_done)

                if not skip:
                    val = cg.llvmCallC("hlt::bytes_sub", [begin_op, next_op])
                else:
                    val = llvm.core.Constant.null(cg.llvmTypeGenericPointer())

                cg.llvmAssign(val, bytes)

                cg.builder().store(next, iter)

                # Leave builder on stack.

            return _unpackFixed

        def unpackRunLength(arg, skip):
            def _unpackRunLength(case):
                # Note: this works only with constant arguments.  I think that's ok
                # but: (FIXME) We should document that somewhere.
                (n, iter) = cg.llvmUnpack(type.Integer(64), begin, end, arg)
                n = operand.LLVM(n, type.Integer(n.type.width))
                return unpackFixed(n, skip, iter)(case)

            return _unpackRunLength

        def unpackDelim(arg, skip):
            def _unpackDelim(case):
                delim = ""
                try:
                    # FIXME: This is set in llvmOp(). We should come up with a
                    # better interface for getting the original value.
                    delim = arg._value
                except AttributeError:
                    pass

                assert isinstance(delim, str) or isinstance(delim, unicode)

                if delim and len(delim) == 1:
                    # We optimize for the case of a single, constant byte.

                    delim = cg.llvmConstInt(ord(delim), 8)
                    block_body = cg.llvmNewBlock("loop-body-start")
                    block_cmp = cg.llvmNewBlock("loop-body-cmp")
                    block_exit = cg.llvmNewBlock("loop-exit")
                    block_insufficient = cg.llvmNewBlock("unpack-out-of-input")

                    # Copy the start iterator.
                    builder = cg.builder()
                    builder.store(begin, iter)

                    # Enter loop.
                    builder.branch(block_body)

                    # Loop body.

                        # Check whether end reached.
                    builder = cg.pushBuilder(block_body)
                    arg1 = operand.LLVM(builder.load(iter), type.IteratorBytes())
                    arg2 = operand.LLVM(end, type.IteratorBytes())
                    done = cg.llvmCallC("hlt::bytes_pos_eq", [arg1, arg2])
                    builder = cg.builder()
                    builder.cbranch(done, block_insufficient, block_cmp)
                    cg.popBuilder()

                        # Check whether we found the delimiter
                    builder = cg.pushBuilder(block_cmp)
                    byte = cg.llvmCallCInternal("__hlt_bytes_extract_one", [iter, end, exception, ctx])
                    done = builder.icmp(llvm.core.IPRED_EQ, byte, delim)
                    builder.cbranch(done, block_exit, block_body)
                    cg.popBuilder()

                        # Not found.
                    builder = cg.pushBuilder(block_insufficient)
                    cg.llvmRaiseExceptionByName("hlt_exception_would_block", self.location())
                    cg.popBuilder()

                    # Loop exit.
                    builder = cg.pushBuilder(block_exit)

                    if not skip:
                        arg1 = operand.LLVM(begin, type.IteratorBytes())
                        arg2 = operand.LLVM(builder.load(iter), type.IteratorBytes())
                        val = cg.llvmCallC("hlt::bytes_sub", [arg1, arg2])
                    else:
                        val = llvm.core.Constant.null(cg.llvmTypeGenericPointer())

                    cg.llvmAssign(val, bytes)

                # Leave builder on stack.

                else:
                    # Everything else we outsource to C.
                    # FIXME: Well, not yet ...
                    util.internal_error("byte unpacking with delimiters only supported for a single constant byte yet")

            return _unpackDelim

        packed = cg.currentModule().scope().lookup("Hilti::Packed")
        assert packed

        cases = [
            ("Hilti::Packed::BytesRunLength", unpackRunLength(arg, False)),
            ("Hilti::Packed::BytesFixed", unpackFixed(arg, False, begin)),
            ("Hilti::Packed::BytesDelim", unpackDelim(arg, False)),

            ("Hilti::Packed::SkipBytesRunLength", unpackRunLength(arg, True)),
            ("Hilti::Packed::SkipBytesFixed", unpackFixed(arg, True, begin)),
            ("Hilti::Packed::SkipBytesDelim", unpackDelim(arg, True)),
            ]

        cg.llvmSwitch(fmt, cases)

        # It's fine to check for an exception at the end.
        cg.llvmExceptionTest(exception)

        return (cg.builder().load(bytes), cg.builder().load(iter))


@hlt.overload(New, op1=cType(cBytes), target=cReferenceOf(cBytes))
class New(Operator):
    """
    Allocates a new *bytes* instancem, which will be initially empty.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_new", [])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Incr, op1=cIteratorBytes, target=cIteratorBytes)
class IterIncr(Operator):
    """
    Advances the iterator to the next element, or the end position
    if already at the last.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_pos_incr", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(IncrBy, op1=cIteratorBytes, op2=cIntegerOfWidth(64), target=cIteratorBytes)
class IterIncrBy(Operator):
    """
    Advances the iterator by *op2* elements, or to the end position
    if that is reached during the process.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_pos_incr_by", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Deref, op1=cIteratorBytes, target=cIntegerOfWidth(8))
class IterDeref(Operator):
    """
    Returns the element the iterator is pointing at.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_pos_deref", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Equal, op1=cIteratorBytes, op2=cIteratorBytes, target=cBool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same position.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_pos_eq", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Equal, op1=cReferenceOf(cBytes), op2=cReferenceOf(cBytes), target=cBool)
class Equal(Operator):
    """Returns True if the content of the two bytes objects are equal.
    """
    def _codegen(self, cg):
        cmp = cg.llvmCallC("hlt::bytes_cmp", [self.op1(), self.op2()])
        zero = cg.llvmConstInt(0, 8)
        result = cg.builder().icmp(llvm.core.IPRED_EQ, cmp, zero)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.length", op1=cReferenceOf(cBytes), target=cIntegerOfWidth(64))
class Length(Instruction):
    """Returns the number of bytes stored in *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_len", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.empty", op1=cReferenceOf(cBytes), target=cBool)
class Empty(Instruction):
    """Returns true if *op1* is empty. Note that using this instruction is
    more efficient than comparing the result of ``bytes.length`` to zero."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_empty", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.append", op1=cReferenceOf(cBytes), op2=cReferenceOf(cBytes))
class Append(Instruction):
    """Appends *op2* to *op1*. The two operands must not refer to the same
    object.

    Raises ValueError if *op1* has been frozen, or if *op1* is the same as
    *op2*.
    """
    def _codegen(self, cg):
        cg.llvmCallC("hlt::bytes_append", [self.op1(), self.op2()])

@hlt.instruction("bytes.sub", op1=cIteratorBytes, op2=cIteratorBytes, target=cReferenceOf(cBytes))
class Sub(Instruction):
    """Extracts the subsequence between *op1* and *op2* from an existing
    *bytes* instance and returns it in a new ``bytes`` instance. The element
    at *op2* is not included."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_sub", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.copy", op1=cReferenceOf(cBytes), target=cReferenceOf(cBytes))
class Copy(Instruction):
    """Copy the contents of *op1* into a new byte instance."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_copy", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.offset", op1=cReferenceOf(cBytes), op2=cIntegerOfWidth(64), target=cIteratorBytes)
class Offset(Instruction):
    """Returns an iterator representing the offset *op2* in *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_offset", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Begin, op1=cReferenceOf(cBytes), target=cIteratorBytes)
class Begin(Operator):
    """Returns an iterator representing the first element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_begin", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.overload(End, op1=cReferenceOf(cBytes), target=cIteratorBytes)
class End(Operator):
    """Returns an iterator representing the position one after the last element of *op1*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_end", [self.op1()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.diff", op1=cIteratorBytes, op2=cIteratorBytes, target=cIntegerOfWidth(64))
class Diff(Instruction):
    """Returns the number of bytes between *op1* and *op2*."""
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_pos_diff", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

#@instruction("bytes.find", op1=Reference, op2=Reference, target=Integer)
#class Find(Instruction):
#    """Searches *op2* in *op1*, returning the (zero-based) start index if it
#    finds it. Returns -1 if it does not find *op2* in *op1*."""
#    pass
#

@hlt.instruction("bytes.cmp", op1=cReferenceOf(cBytes), op2=cReferenceOf(cBytes), target=cIntegerOfWidth(8))
class Cmp(Instruction):
    """Compares *op1* with *op2* lexicographically. If *op1* is larger,
    returns -1. If both are equal, returns 0. If *op2* is larger, returns 1.
    """
    def _codegen(self, cg):
        result = cg.llvmCallC("hlt::bytes_cmp", [self.op1(), self.op2()])
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("bytes.freeze", op1=cReferenceOf(cBytes))
class Freeze(Instruction):
    """Freezes the bytes object *op1*. A frozen bytes object cannot be further
    modified until unfrozen. If the object is already frozen, the instruction
    is ignored.
    """
    def _codegen(self, cg):
        freeze = constant.Constant(1, type.Bool())
        cg.llvmCallC("hlt::bytes_freeze", [self.op1(), operand.Constant(freeze)])

@hlt.instruction("bytes.unfreeze", op1=cReferenceOf(cBytes))
class Unfreeze(Instruction):
    """Unfreezes the bytes object *op1*. An unfrozen bytes object can be
    further modified. If the object is already unfrozen (which is the
    default), the instruction is ignored.
    """
    def _codegen(self, cg):
        freeze = constant.Constant(0, type.Bool())
        cg.llvmCallC("hlt::bytes_freeze", [self.op1(), operand.Constant(freeze)])

@hlt.instruction("bytes.trim", op1=cReferenceOf(cBytes), op2=cIteratorBytes)
class Trim(Instruction):
    """Trims the bytes object *op1* at the beginning by removing data up to
    (but not including) the given position *op2*. The iterator *op2* will
    remain valid afterwards and still point to the same location, which will
    now be the first of the bytes object.

    Note: The effect of this instruction is undefined if the given iterator
    *op2* does not actually refer to a location inside the bytes object *op1*.
    """
    def _codegen(self, cg):
        freeze = constant.Constant(0, type.Bool())
        cg.llvmCallC("hlt::bytes_trim", [self.op1(), self.op2()])

@hlt.constraint("ref<bytes> or iterator<bytes>")
def _bytesOrIterator(ty, op, i):
    (bytes, msg) = cIteratorBytes(ty, op, i)
    (iter, msg) = cReferenceOf(cBytes)(ty, op, i)
    return (bytes or iter, "must be ref<bytes> or iterator<bytes>")

@hlt.instruction("bytes.is_frozen", op1=_bytesOrIterator, target=cBool)
class IsFrozen(Instruction):
    """Returns whether the bytes object *op1* (or the bytes objects referred
    to be the iterator *op1*) has been frozen.
    """
    def _codegen(self, cg):
        if isinstance(self.op1().type(), type.Reference):
            result = cg.llvmCallC("hlt::bytes_is_frozen", [self.op1()])
        else:
            result = cg.llvmCallC("hlt::bytes_pos_is_frozen", [self.op1()])

        cg.llvmStoreInTarget(self, result)

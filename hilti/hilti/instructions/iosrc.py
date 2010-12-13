# $Id$
"""
The *iosrc* data type represents a source of external input coming in for
processing. It transparently support a set of different sources (currently
only ``libpcap``-based, but in the future potentially other's as well.).
Elements of an IOSrc are ``bytes`` objects and come with a timestamp.
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

import bytes

class Kind:
    # We map these to the corresponding enum values.
    Any = "*"
    PcapLive = "Hilti::IOSrc::PcapLive"
    PcapOffline = "Hilti::IOSrc::PcapOffline"

@hlt.type(None, 103)
class IteratorIOSrc(type.Iterator):
    """Type for iterating over the packets provided by a ``IOSrc``.

    t: ~~IOSrc - The type of the packet source.

    location: ~~Location - Location information for the type.
    """
    def __init__(self, t, location=None):
        super(IteratorIOSrc, self).__init__(t, location=location)

    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_iosrc*"
        return typeinfo

    def llvmType(self, cg):
        """A ``IOSrc`` object is mapped to a struct of ``hlt_iosrc *`` and
        the ``tuple<double, ref<bytes>``."""
        return llvm.core.Type.struct([cg.llvmTypeGenericPointer(), cg.llvmType(self.derefType())])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """A ``IOSrc`` iterator is initially set to the end of (any)
        ``IOSrc`` object."""
        return _makeIterator(cg, self.parentType(), None, None)

    ### Overridden from Iterator.

    def derefType(self):
        return type.Tuple([type.Double(), type.Reference(type.Bytes())])

@hlt.type("iosrc", 27)
class IOSrc(type.HeapType, type.Parameterizable, type.Iterable):
    """Type for ``IOSrc``.

    kind: ~~Kind - The kind of packet source.

    Note: The container's item type is ``tuple<double,ref<bytes>``.
    """
    def __init__(self, kind, location=None):
        super(IOSrc, self).__init__(location=location)
        self._kind = kind if kind else Kind.Any

    def kind(self):
        """Returns the kind of packet source.

        Returns: ~~Kind - The kind.
        """
        return self._kind

    ### Overridden from Type.

    def name(self):
        return "iosrc<%s>" % self._kind

    ### Overridden from HiltiType.

    def llvmType(self, cg):
        """A ``iosrc`` object is mapped to ``hlt_iosrc *``."""
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_iosrc *"
        typeinfo.to_string = "hlt::iosrc_pcap_to_string"
        return typeinfo

    def canCoerceTo(self, dsttype):
        if not isinstance(dsttype, IOSrc):
            return False

        if dsttype.kind() == Kind.Any:
            return True

        return self.kind() == dsttype.kind()

    def llvmCoerceTo(self, cg, value, dsttype):
        assert self.canCoerceTo(dsttype)
        return value

    ### Overridden from Parameterizable.

    def args(self):
        return [self._kind]

    ### Overriden from Iterable.

    def iterType(self):
        return IteratorIOSrc(self)

_prefixes = {
    Kind.PcapLive: ("pcap", "live"),
    Kind.PcapOffline: ("pcap", "offline")
    }

def _funcName(cfunc, kind, ctor=False):
    (prefix, addl) = _prefixes[kind]
    func = "hlt::iosrc_%s_%s" % (prefix, cfunc)
    if ctor:
        func += "_%s" % addl
    return func

def _makeSwitch(cg, ty, cfunc, args, result, ctor=None):
    def _case(key):
        def __case(op):
            return cg.llvmCallC(_funcName(cfunc, key, ctor), args)
        return __case

    cases = [(key, _case(key)) for key in _prefixes]
    return cg.llvmSwitch(ty.kind(), cases, result=result)

def _checkExhausted(cg, i, src, result, make_iters=False):
    ty = src.type().refType()
    data = cg.llvmExtractValue(result, 1)
    exhausted = cg.builder().icmp(llvm.core.IPRED_EQ, data, llvm.core.Constant.null(data.type))

    block_exhausted = cg.llvmNewBlock("excpt")
    block_notexhausted = cg.llvmNewBlock("noexcpt")
    block_cont = cg.llvmNewBlock("cont")
    cg.builder().cbranch(exhausted, block_exhausted, block_notexhausted)

    cg.pushBuilder(block_exhausted)

    if not make_iters:
        cg.llvmRaiseExceptionByName("hlt_exception_iosrc_exhausted", i.location())
    else:
        result_exhausted = _makeIterator(cg, ty, None, None)

    cg.builder().branch(block_cont)
    cg.popBuilder()

    # We leave this builder for subseqent code.
    cg.pushBuilder(block_notexhausted)
    if make_iters:
        result_notexhausted = _makeIterator(cg, ty, cg.llvmOp(src), result)

    cg.builder().branch(block_cont)
    cg.popBuilder()

    cg.pushBuilder(block_cont)

    if make_iters:
        result = cg.builder().phi(result_exhausted.type)
        result.add_incoming(result_exhausted, block_exhausted)
        result.add_incoming(result_notexhausted, block_notexhausted)
        return result

    else:
        return None

    # Leave builder.

def _makeIterator(cg, ty, src, elem):
    if not src:
        src = llvm.core.Constant.null(ty.llvmType(cg))

    if not elem:
        elem = llvm.core.Constant.null(cg.llvmType(ty.iterType().derefType()))

    iter = llvm.core.Constant.null(cg.llvmType(ty.iterType()))
    iter = cg.llvmInsertValue(iter, 0, src)
    iter = cg.llvmInsertValue(iter, 1, elem)
    return iter

@hlt.overload(New, op1=cType(cIOSrc), op2=cString, target=cReferenceOf(cIOSrc))
class New(Operator):
    """Instantiates a new *iosrc* instance, and initializes it for reading.
    The format of string *op2* is depending on the kind of ``iosrc``. For
    :hlt:glob:`PcapLive`, it is the name of the local interface to listen
    on. For :hlt:glob:`PcapLive`, it is the name of the trace file.

    Raises: :hlt:type:`IOSrcError` if the packet source cannot be opened.
    """
    def _codegen(self, cg):
        result = _makeSwitch(cg, self.op1().value(), "new", [self.op2()], True, ctor=True)
        assert result
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("iosrc.read", op1=cReferenceOf(cIOSrc), target=cIsTuple([cDouble, cReferenceOf(cBytes)]))
class Read(BlockingInstruction):
    """Returns the next element from the I/O source *op1*. If currently no
    element is available, the instruction blocks until one is.

    Raises: ~~IOSrcExhausted if the source has been exhausted.

    Raises: ~~IOSrcError if there is any other problem with returning the next
    element.
    """
    def cCall(self, cg):
        func = _funcName("read_try", self.op1().type().refType().kind())
        args = [self.op1(), operand.Constant(constant.Constant(0, type.Bool()))]
        return (func, args)

    def cResult(self, cg, result):
        _checkExhausted(cg, self, self.op1(), result)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("iosrc.close", op1=cReferenceOf(cIOSrc))
class Close(Instruction):
    """Closes the packet source *op1*. No further packets can then be read (if
    tried, ``pkrsrc.read`` will raise a ~~IOSrcError exception).
    """
    def _codegen(self, cg):
        _makeSwitch(cg, self.op1().type().refType(), "close", [self.op1()], False)

@hlt.overload(Begin, op1=cReferenceOf(cIOSrc), target=cIteratorIOSrc)
class Begin(BlockingOperator):
    """Returns an iterator for iterating over all elements of a packet source
    *op1*. The instruction will block until the first element becomes
    available."""
    def _canonify(self, canonifier):
        Instruction._canonify(self, canonifier)
        self.blockingCanonify(canonifier)

    def _codegen(self, cg):
        self.blockingCodegen(cg)

    def cCall(self, cg):
        func = _funcName("read_try", self.op1().type().refType().kind())
        args = [self.op1(), operand.Constant(constant.Constant(0, type.Bool()))]
        return (func, args)

    def cResult(self, cg, result):
        result = _checkExhausted(cg, self, self.op1(), result, make_iters=True)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(End, op1=cReferenceOf(cIOSrc), target=cIteratorIOSrc)
class End(Operator):
    """Returns an iterator representing an exhausted packet source *op1*."""
    def _codegen(self, cg):
        ty = self.op1().type().refType()
        result = _makeIterator(cg, ty, None, None)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Incr, op1=cIteratorIOSrc, target=cIteratorIOSrc)
class Incr(BlockingOperator):
    """Advances the iterator to the next element, or the end position if
    already exhausted. The instruction will block until the next element
    becomes available.
    """
    def _canonify(self, canonifier):
        Instruction._canonify(self, canonifier)
        self.blockingCanonify(canonifier)

    def _codegen(self, cg):
        self.blockingCodegen(cg)

    def cCall(self, cg):
        ty = self.op1().type().parentType()
        src = operand.LLVM(cg.llvmExtractValue(cg.llvmOp(self.op1()), 0), type.Reference(ty))
        func = _funcName("read_try", ty.kind())
        args = [src, operand.Constant(constant.Constant(0, type.Bool()))]
        return (func, args)

    def cResult(self, cg, result):
        ty = self.op1().type().parentType()
        src = operand.LLVM(cg.llvmExtractValue(cg.llvmOp(self.op1()), 0), type.Reference(ty))
        result = _checkExhausted(cg, self, src, result, make_iters=True)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Deref, op1=cIteratorIOSrc, target=cDerefTypeOfOp(1))
class Deref(Operator):
    """Returns the element the iterator is pointing at as a tuple ``(double,
    ref<bytes>)``.
    """
    def _codegen(self, cg):
        elem = cg.llvmExtractValue(cg.llvmOp(self.op1()), 1)
        cg.llvmStoreInTarget(self, elem)

@hlt.overload(Equal, op1=cIteratorIOSrc, op2=cIteratorIOSrc, target=cBool)
class Equal(Operator):
    """Returns true if *op1* and *op2* refer to the same element.
    """
    def _codegen(self, cg):
        elem1 = cg.llvmExtractValue(cg.llvmOp(self.op1()), 1)
        payload1 = cg.llvmExtractValue(elem1, 1)
        elem2 = cg.llvmExtractValue(cg.llvmOp(self.op2()), 1)
        payload2 = cg.llvmExtractValue(elem2, 1)
        result = cg.builder().icmp(llvm.core.IPRED_EQ, payload1, payload2)
        cg.llvmStoreInTarget(self, result)


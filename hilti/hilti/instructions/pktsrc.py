# $Id$
"""
Packet Sources
~~~~~~~~~~~~~~

The *pktsrc* data type represents a source of network packets
coming in for processing. It transparently support a set of
different sources (currently only ``libpcap``-based, but in the
future potentially other's as well.). 
"""

import llvm.core

import hilti.util as util

from hilti.constraints import *
from hilti.instructions.operators import *

import bytes

class Kind:
    # We map these to the corresponding enum values.
    Any = "*"
    PcapLive = "Hilti::PktSrc::PcapLive"
    PcapOffline = "Hilti::PktSrc::PcapOffline"

@hlt.type(None, 103)
class IteratorPktSrc(type.Iterator):
    """Type for iterating over the packets provided by a ``pktsrc``.
    
    t: ~~PktSrc - The type of the packet source.
    
    location: ~~Location - Location information for the type.
    """
    def __init__(self, t, location=None):
        super(IteratorPktSrc, self).__init__(t, location=location)
        
    ### Overridden from HiltiType.

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_pkt_src*"
        return typeinfo
    
    def llvmType(self, cg):
        """A ``pktsrc`` object is mapped to a struct of ``hlt_pktsrc *`` and
        the ``tuple<double, ref<bytes>``."""
        return llvm.core.Type.struct([cg.llvmTypeGenericPointer(), cg.llvmType(self.derefType())])

    ### Overridden from ValueType.

    def llvmDefault(self, cg):
        """A ``pktsrc`` iterator is initially set to the end of (any)
        ``pktsrc`` object."""
        return _makeIterator(cg, self.parentType(), None, None)

    ### Overridden from Iterator.
    
    def derefType(self):
        return type.Tuple([type.Double(), type.Reference(type.Bytes())])
    
@hlt.type("pktsrc", 27)
class PktSrc(type.HeapType, type.Parameterizable, type.Iterable):
    """Type for ``pktsrc``. 
    
    kind: ~~Kind - The kind of packet source. 
    
    Note: The container's item type is ``tuple<double,ref<bytes>``. 
    """
    def __init__(self, kind, location=None):
        super(PktSrc, self).__init__(location=location)
        self._kind = kind if kind else Kind.Any

    def kind(self):
        """Returns the kind of packet source.
        
        Returns: ~~Kind - The kind.
        """
        return self._kind
        
    ### Overridden from Type.

    def name(self):
        return "pktsrc<%s>" % self._kind

    def output(self, printer):
        printer.output(self.name())

        if not dsttype._type:
            return True
        
        return self._type == dsttype._type
        
    ### Overridden from HiltiType.
    
    def llvmType(self, cg):
        """A ``pktsrc`` object is mapped to ``hlt_pktsrc *``."""
        return cg.llvmTypeGenericPointer()

    def typeInfo(self, cg):
        typeinfo = cg.TypeInfo(self)
        typeinfo.c_prototype = "hlt_pktsrc *"
        typeinfo.to_string = "hlt::pktsrc_to_string"
        return typeinfo

    def canCoerceTo(self, dsttype):
        if not isinstance(dsttype, PktSrc):
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
        return IteratorPktSrc(self)
    
_postfixes = {
    Kind.PcapLive: "pcap_live",
    Kind.PcapOffline: "pcap_offline"
    }
    
def _makeSwitch(cg, ty, cfunc, args, result):
    def _case(postfix):
        def __case(op):
            return cg.llvmCallC("hlt::pktsrc_%s_%s" % (cfunc, postfix), args)
        return __case
                
    cases = [(key, _case(postfix)) for (key, postfix) in _postfixes.items()]
    return cg.llvmSwitch(ty.kind(), cases, result=result)

def _nextPacket(cg, ty, src, block, keep_link, exhausted_excpt, instr):
    true = constant.Constant(1, type.Bool())
    false = constant.Constant(0, type.Bool())
    keep_link = keep_link if keep_link else operand.Constant(false)
    block = block if block else operand.Constant(true)
    result = _makeSwitch(cg, ty, "read", [src, block, keep_link], True)
    
    if exhausted_excpt:
        # Check whether the source is exhausted. 
        data = cg.llvmExtractValue(result, 1)
        exhausted =  cg.builder().icmp(llvm.core.IPRED_EQ, data, llvm.core.Constant.null(data.type))
    
    	block_excpt = cg.llvmNewBlock("excpt")
        block_noexcpt= cg.llvmNewBlock("noexcpt")
        cg.builder().cbranch(exhausted, block_excpt, block_noexcpt)
    
        cg.pushBuilder(block_excpt)
        cg.llvmRaiseExceptionByName("hlt_exception_pktsrc_exhausted", instr.location())
        cg.popBuilder()
    
        # We leave this builder for subseqent code.
        cg.pushBuilder(block_noexcpt)
        
    return result

def _makeIterator(cg, ty, src, pkt):
    if not src:
        src = llvm.core.Constant.null(ty.llvmType(cg))
        
    if not pkt:
        pkt = llvm.core.Constant.null(cg.llvmType(ty.iterType().derefType()))
        
        
    iter = llvm.core.Constant.null(cg.llvmType(ty.iterType()))
    iter = cg.llvmInsertValue(iter, 0, src)
    iter = cg.llvmInsertValue(iter, 1, pkt)
    return iter

@hlt.overload(New, op1=cType(cPktSrc), op2=cString, target=cReferenceOf(cPktSrc))
class New(Operator):
    """Instantiates a new *pktsrc* instance, and initializes it for reading on
    interface *op2*. The format of the interface string is depending on the
    kind of ``pkrsrc``. For ~~PcapLive, it is the name of the local interface
    to listen on. For ~~PcapOffline, it is the name of the trace file. 
    
    Raises: ~~PktSrcError if the packet source cannot be opened. 
    """
    def codegen(self, cg):
        result = _makeSwitch(cg, self.op1().value(), "new", [self.op2()], True)
        assert result
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("pktsrc.read", op1=cReferenceOf(cPktSrc), op2=cOptional(cBool), target=cIsTuple([cDouble, cReferenceOf(cBytes)]))
class Read(Instruction):
    """Returns the next packet from the packet source *op1*. If currently no
    packet is available, the function blocks until one is. If *op2* is given
    and set to True, the packet will be return including the link-layer
    header. If *op2* is not given, the link-layer header will have been
    removed.
    
    Raises: ~~PktSrcExhausted if the source has been exhausted.  
    
    Raises: ~~PktSrcError if there is any other problem with returning the
    next packet, including encoutering a non-known link-layer if stripping
    them is specified. 
    """
    def codegen(self, cg):
        keep_link = self.op2() if self.op2() else None
        result = _nextPacket(cg, self.op1().type().refType(), self.op1(), None, keep_link, True, self)
        cg.llvmStoreInTarget(self, result)

@hlt.instruction("pktsrc.close", op1=cReferenceOf(cPktSrc))
class Close(Instruction):
    """Closes the packet source *op1*. No further packets can then be read (if
    tried, ``pkrsrc.read`` will raise a ~~PktSrcError exception). 
    """
    def codegen(self, cg):
        _makeSwitch(cg, self.op1().type().refType(), "close", [self.op1()], False)
        
@hlt.overload(Begin, op1=cReferenceOf(cPktSrc), target=cIteratorPktSrc)
class Begin(Operator):
    """Returns an iterator for iterating over all packets of packet source
    *op1*. The instruction will block until the first packet becomes
    available."""
    def codegen(self, cg):
        ty = self.op1().type().refType()
        src = self.op1()
        pkt = _nextPacket(cg, ty, src, None, None, True, self)
        result = _makeIterator(cg, ty, cg.llvmOp(src), pkt)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(End, op1=cReferenceOf(cPktSrc), target=cIteratorPktSrc)
class End(Operator):
    """Returns an iterator representing an exhausted packet source *op1*."""
    def codegen(self, cg):
        ty = self.op1().type().refType()
        result = _makeIterator(cg, ty, None, None)
        cg.llvmStoreInTarget(self, result)

@hlt.overload(Incr, op1=cIteratorPktSrc, target=cIteratorPktSrc)
class IterIncr(Operator):
    """ Advances the iterator to the next packet, or the end position if
    already exhausted. The instruction will block until the next packets
    becomes available.
    """
    def codegen(self, cg):
        ty = self.op1().type().parentType()        
        src = operand.LLVM(cg.llvmExtractValue(cg.llvmOp(self.op1()), 0), type.Reference(ty))
        pkt = _nextPacket(cg, self.op1().type().parentType(), src, None, None, False, self)
        
        # Check whether we have exhausted the source.
        data = cg.llvmExtractValue(pkt, 1)
        exhausted = cg.builder().icmp(llvm.core.IPRED_EQ, data, llvm.core.Constant.null(data.type))
    
    	block_exhausted = cg.llvmNewBlock("exhausted")
        block_notexhausted = cg.llvmNewBlock("not-exhausted")
        block_cont = cg.llvmNewBlock("cont")
        cg.builder().cbranch(exhausted, block_exhausted, block_notexhausted)
    
        cg.pushBuilder(block_exhausted)
        result_exhausted = _makeIterator(cg, ty, None, None)
        cg.builder().branch(block_cont)
        cg.popBuilder()
    
        cg.pushBuilder(block_notexhausted)
        result_notexhausted = _makeIterator(cg, ty, cg.llvmOp(src), pkt)
        cg.builder().branch(block_cont)
        cg.popBuilder()

        cg.pushBuilder(block_cont)
        result = cg.builder().phi(result_exhausted.type)
        result.add_incoming(result_exhausted, block_exhausted)
        result.add_incoming(result_notexhausted, block_notexhausted)
        cg.llvmStoreInTarget(self, result)
        
        # Leave builder on stack.

@hlt.overload(Deref, op1=cIteratorPktSrc, target=cDerefTypeOfOp(1))
class IterDeref(Operator):
    """ Returns the packet the iterator is pointing at as a tuple ``(double,
    ref<bytes>)``. The link-layer header will have been removed.
    """
    def codegen(self, cg):
        pkt = cg.llvmExtractValue(cg.llvmOp(self.op1()), 1)
        cg.llvmStoreInTarget(self, pkt)

@hlt.overload(Equal, op1=cIteratorPktSrc, op2=cIteratorPktSrc, target=cBool)
class IterEqual(Operator):
    """
    Returns true if *op1* and *op2* specify the same packet.
    """
    def codegen(self, cg):
        pkt1 = cg.llvmExtractValue(cg.llvmOp(self.op1()), 1)
        payload1 = cg.llvmExtractValue(pkt1, 1)
        pkt2 = cg.llvmExtractValue(cg.llvmOp(self.op2()), 1)
        payload2 = cg.llvmExtractValue(pkt2, 1)
        result = cg.builder().icmp(llvm.core.IPRED_EQ, payload1, payload2)
        cg.llvmStoreInTarget(self, result)


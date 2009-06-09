# $Id$
"""
Network Prefixes
~~~~~~~~~~~~~~~~
"""

_doc_type_description = """The *net* data type represents ranges of IP
address, specified in CIDR notation. It transparently handles both IPv4 and
IPv6 networks. Network constants are specified in standard CIDR notation for
either v4 addresses ("dotted quad", e.g., ``192.168.1.0/24``), or v6 addresses
(colon-notation, e.g., ``2001:db8::1428:57ab/48``). If not explicitly
initialized, an network is set to ``::0/0``."""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(Equal, op1=net, op2=net, target=bool)
class EqualNet(Operator):
    """ Returns True if the networks in *op1* and *op2* cover the same IP
    range. Note that an IPv4 network ``a.b.c.d/n`` will match the corresponding
    IPv6 ``::a.b.c.d/n``.
    
    Todo: Are the v6 vs v4 matching semantics right? Should it be
    ``::ffff:a.b.c.d``? Should v4 never match a v6 address?``
    """
    pass

@overload(Equal, op1=net, op2=addr, target=bool)
class EqualAddr(Operator):
    """Returns True if the addr *op2* is part of the network *op1*."""
    pass


@instruction("net.family", op1=net, target=enum)
class Family(Instruction):
    """
    Returns the address family of *op1*, which can currently be either
    ``AddrFamily::IPv4`` or ``AddrFamiliy::IPv6``. 
    """

@instruction("net.prefix", op1=net, target=addr)
class Prefix(Instruction):
    """
    Returns the network prefix as a masked IP addr.
    """

@instruction("net.length", op1=net, target=integerOfWidth(8))
class Length(Instruction):
    """
    Returns the length of the network prefix.
    """
    

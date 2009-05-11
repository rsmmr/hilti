# $Id$
"""
IP Addresses
~~~~~~~~~~~~
"""

_doc_type_description = """The *addr* data type represents IP addresses. It
transparently handles both IPv4 and IPv6 addresses. Address constants are
specified in standartd notation for either v4 addresses ("dotted quad", e.g.,
``192.168.1.1``), or v6 addresses (colon-notation, e.g.,
``2001:db8::1428:57ab``). If not explicitly initialized, an address is set to
``::0``.  """

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(Equal, op1=addr, op2=addr, target=bool)
class Equal(Operator):
    """ Returns True if the address in *op1* equals the address in *op2*. Note
    that an IPv4 address ``a.b.c.d`` will match the corresponding IPv6
    ``::a.b.c.d``.
    
    Todo: Are the v6 vs v4 matching semantics right? Should it be
    ``::ffff:a.b.c.d``? Should v4 never match a v6 address?``
    """
    pass

@instruction("addr.family", op1=addr, target=enum)
class Family(Instruction):
    """
    Returns the address family of *op1*, which can currently be either
    ``AddrFamily::IPv4`` or ``AddrFamiliy::IPv6``. 
    """
    

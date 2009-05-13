# $Id$
"""
Port numbers 
~~~~~~~~~~~~
"""

_doc_type_description = """The *port* data type represents TCP and UDP port
numbers. Port constants are written with a ``/tcp`` or ``/udp`` prefix,
respectively. For example, ``80/tcp`` and ``53/udp``. If not explicitly
initialized, a port ``0/tcp``."""

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(Equal, op1=port, op2=port, target=bool)
class Equal(Operator):
    """Returns True if the port in *op1* equals the port in *op2*. Note that
    a TCP port will *not* match the UDP port of the same value.
    """
    pass

@instruction("port.protocol", op1=port, target=enum)
class Protocol(Instruction):
    """
    Returns the protocol of *op1*, which can currently be either
    ``Port::TCP`` or ``Port::UDP``. 
    """
    

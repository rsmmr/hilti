# $Id$
"""
Channels
~~~~~~~~
"""

_doc_type_description = """A channel is a high-performance data type for I/O
operations that is designed to efficiently transfer large volumes of data both
between the host application and HILTI and intra-HILTI components. 

The channel behavior depends on its type parameters which enable fine-tuning
along the following dimensions:
* Capacity. The capacity represents the maximum number of items a channel can
  hold. A full bounded channel cannot accomodate further items and a reader must
  first consume an item to render the channel writable again. By default,
  channels are unbounded and can grow arbitrarily large.  
* Blocking behavior. Channels operate either in *blocking* or *non-blocking*
  mode. In blocking mode, reading/writing of an empty/full channel suspends the
  caller and blocks until a channel item is inserted/removed. In non-blocking
  mode, read/write operations return immediately when the channel is empty/full
  and throw an exception describing the error. The default mode of operation is
  non-blocking for both writes and reads.
"""

from hilti.core.type import *
from hilti.core.instruction import *

# FIXME: Technically the new instruction does not need any parameters add all
# because it should infer them from the channel type information. However, 
# we have to duplicate the type information in the form of instruction operands
# because the instruction signature is transparently mapped to a libhilti C
# function. Ideally, a generic new instruction passes the channel type
# information as __hlt_type_info* pointer as parameter which would obviate the
# need for this duplication.
@instruction("channel.new", op1=Type, op2=ValueType, target=Reference)
class New(Instruction):
    """Instantiates a new channel object of the type *op1* with capacity *op2*
    and returns a reference to it.
    """

@instruction("channel.write", op1=Reference, op2=ValueType)
class Write(Instruction):
    """Writes an item into the channel referenced by *op1*. If the channel is
    full, the caller blocks.
    """

@instruction("channel.read", op1=Reference, target=ValueType)
class Read(Instruction):
    """Returns the next channel item from the channel referenced by *op1*. If
    the channel is empty, the caller blocks.
    """

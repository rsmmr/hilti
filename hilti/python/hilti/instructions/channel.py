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

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@overload(New, op1=isType(channel), target=referenceOfOp(1))
class New(Operator):
    """Allocates a new instance of a channel with the same type as the target
    Reference.
    """
    pass

@instruction("channel.write", op1=referenceOf(channel), op2=itemTypeOfOp(1))
class Write(Instruction):
    """Writes an item into the channel referenced by *op1*. If the channel is
    full, the caller blocks.
    """

@instruction("channel.read", op1=referenceOf(channel), target=itemTypeOfOp(1))
class Read(Instruction):
    """Returns the next channel item from the channel referenced by *op1*. If
    the channel is empty, the caller blocks.
    """

@instruction("channel.size", op1=referenceOf(channel), target=integerOfWidth(64))
class Size(Instruction):
    """Returns the current number of items in the channel referenced by *op1*.
    """

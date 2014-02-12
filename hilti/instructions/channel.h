///
/// \type Channel
///
/// A channel is a high-performance data type for I/O operations that is
/// designed to efficiently transfer large volumes of data both between the
/// host application and HILTI and intra-HILTI components.
///
/// The channel behavior depends on its type parameters which enable
/// fine-tuning along the following dimensions:
///
/// * Capacity. The capacity represents the maximum number of items a channel
///         can hold. A full bounded channel cannot accomodate further items
///         and a reader must first consume an item to render the channel
///         writable again. By default, channels are unbounded and can grow
///         arbitrarily large.
///
/// \cproto hlt_channel*
///

#include "define-instruction.h"

iBeginH(channel, New, "new")
    iTarget(optype::refChannel)
    iOp1(optype::typeChannel, true);
    iOp2(optype::optional(optype::int64), true);
iEndH

iBeginH(channel, Read, "channel.read")
    iTarget(optype::any)
    iOp1(optype::refChannel, true)
iEndH

iBeginH(channel, ReadTry, "channel.read_try")
    iTarget(optype::any)
    iOp1(optype::refChannel, true)
iEndH

iBeginH(channel, Size, "channel.size")
    iTarget(optype::int64)
    iOp1(optype::refChannel, true)
iEndH

iBeginH(channel, Write, "channel.write")
    iOp1(optype::refChannel, false)
    iOp2(optype::any, false)
iEndH

iBeginH(channel, WriteTry, "channel.write_try")
    iOp1(optype::refChannel, false)
    iOp2(optype::any, false)
iEndH


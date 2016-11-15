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


iBegin(channel::New, "new")
    iTarget(optype::refChannel);
    iOp1(optype::typeChannel, true);
    iOp2(optype::optional(optype::int64), true);

    iValidate {
        equalTypes(referencedType(target), typedType(op1));
    }

    iDoc(R"(
         Allocates a new instance of a channel storing elements of type *op1*.
         *op2* defines the channel's capacity, i.e., the maximal number of items
         it can store. The capacity defaults to zero, which creates a channel of
         unbounded capacity.
    )");
iEnd

iBegin(channel::Read, "channel.read")
    iTarget(optype::any);
    iOp1(optype::refChannel, true);

    iValidate {
        canCoerceTo(argType(op1), target);
    }

    iDoc(R"(
        Returns the next channel item from the channel referenced by *op1*.
        If the channel is empty, the instruction blocks until an item becomes
        available.
    )");
iEnd

iBegin(channel::ReadTry, "channel.read_try")
    iTarget(optype::any);
    iOp1(optype::refChannel, true);

    iValidate {
        canCoerceTo(argType(op1), target);
    }

    iDoc(R"(
        Returns the next channel item from the channel referenced by *op1*. If
        the channel is empty, the instruction raises a ``WouldBlock``
        exception.
    )");
iEnd

iBegin(channel::Size, "channel.size")
    iTarget(optype::int64);
    iOp1(optype::refChannel, true);

    iValidate {
    }

    iDoc(R"(
        Returns the current number of items in the channel referenced by *op1*.
    )");
iEnd

iBegin(channel::Write, "channel.write")
    iOp1(optype::refChannel, false);
    iOp2(optype::any, false);

    iValidate {
        canCoerceTo(op2, argType(op1));
    }

    iDoc(R"(
        Writes an item into the channel referenced by *op1*. If the channel is
        full, the instruction blocks until a slot becomes available.
    )");
iEnd

iBegin(channel::WriteTry, "channel.write_try")
    iOp1(optype::refChannel, false);
    iOp2(optype::any, false);

    iValidate {
        canCoerceTo(op2, argType(op1));
    }

    iDoc(R"(
        Writes an item into the channel referenced by *op1*. If the channel is
        full, the instruction raises a ``WouldBlock`` exception.
    )");
iEnd

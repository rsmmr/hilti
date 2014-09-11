///
/// \type Bytes
///
/// *bytes* is a data type for storing sequences of raw bytes. It is
/// optimized for storing and operating on large amounts of unstructured
/// data. In particular, it provides efficient subsequence and append
/// operations. Bytes are forward-iterable.
///
/// \default *bytes* instances default to being empty.
///
/// \ctor The constructor for ``bytes`` resembles the one for strings:
/// ``b"abcdef"`` creates a new ``bytes`` object and initialized it with six
/// bytes. Note that such ``bytes`` instances are *not* constants but can be
/// modified later on.
///
/// \cproto hlt_bytes*
///
/////
///
/// \type ``iter<bytes>``
///
/// \default A ``bytes`` iterator is initially set to the end of (any)
/// ``bytes`` object.
///
/// \cproto hlt_bytes_iter
///

#include "define-instruction.h"

iBeginH(bytes, New, "new")
    iTarget(optype::refBytes)
    iOp1(optype::typeBytes, true);
iEndH

iBeginH(bytes, Append, "bytes.append")
    iOp1(optype::refBytes, false);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Concat, "bytes.concat")
    iTarget(optype::refBytes)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Cmp, "bytes.cmp")
    iTarget(optype::int8)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Copy, "bytes.copy")
    iTarget(optype::refBytes)
    iOp1(optype::refBytes, true);
iEndH

iBeginH(bytes, Diff, "bytes.diff")
    iTarget(optype::int64);
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);
iEndH

iBeginH(bytes, Empty, "bytes.empty")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);
iEndH

iBeginH(bytes, Freeze, "bytes.freeze")
    iOp1(optype::refBytes, false);
iEndH

iBeginH(bytes, IsFrozenBytes, "bytes.is_frozen")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);
iEndH

iBeginH(bytes, IsFrozenIterBytes, "bytes.is_frozen")
    iTarget(optype::boolean);
    iOp1(optype::iterBytes, true);
iEndH

iBeginH(bytes, Length, "bytes.length")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);
iEndH

iBeginH(bytes, Contains, "bytes.contains")
    iTarget(optype::boolean);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Find, "bytes.find")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, FindAtIter, "bytes.find")
    iTarget(optype::tuple);
    iOp1(optype::iterBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Offset, "bytes.offset")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);
    iOp2(optype::int64, true);
iEndH

iBeginH(bytes, Sub, "bytes.sub")
    iTarget(optype::refBytes);
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);
iEndH

iBeginH(bytes, Trim, "bytes.trim")
    iOp1(optype::refBytes, false);
    iOp2(optype::iterBytes, true);
iEndH

iBeginH(bytes, Unfreeze, "bytes.unfreeze")
    iOp1(optype::refBytes, false);
iEndH

iBeginH(bytes, ToIntFromAscii, "bytes.to_int")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);
    iOp2(optype::int64, true);
iEndH

iBeginH(bytes, ToIntFromBinary, "bytes.to_int")
    iTarget(optype::int64);
    iOp1(optype::refBytes, true);
    iOp2(optype::enum_, true);
iEndH

iBeginH(bytes, Lower, "bytes.lower")
    iTarget(optype::refBytes);
    iOp1(optype::refBytes, true);
iEndH

iBeginH(bytes, Upper, "bytes.upper")
    iTarget(optype::refBytes)
    iOp1(optype::refBytes, true);
iEndH

iBeginH(bytes, Strip, "bytes.strip")
    iTarget(optype::refBytes)
    iOp1(optype::refBytes, true);
    iOp2(optype::optional(optype::enum_), true);
    iOp3(optype::optional(optype::refBytes), true);
iEndH

iBeginH(bytes, StartsWith, "bytes.startswith")
    iTarget(optype::boolean)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Split1, "bytes.split1")
    iTarget(optype::tuple)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Split, "bytes.split")
    iTarget(optype::refVector)
    iOp1(optype::refBytes, true);
    iOp2(optype::refBytes, true);
iEndH

iBeginH(bytes, Join, "bytes.join")
    iTarget(optype::refBytes)
    iOp1(optype::refBytes, true);
    iOp2(optype::refList, true);
iEndH

iBeginH(bytes, AppendObject, "bytes.append_object")
    iOp1(optype::refBytes, false);
    iOp2(optype::any, true);
iEndH

iBeginH(bytes, RetrieveObject, "bytes.retrieve_object")
    iTarget(optype::any)
    iOp1(optype::iterBytes, true);
iEndH

iBeginH(bytes, AtObject, "bytes.at_object")
    iTarget(optype::boolean)
    iOp1(optype::iterBytes, true);
    iOp2(optype::optional(optype::typeAny), true);
iEndH

iBeginH(bytes, SkipObject, "bytes.skip_object")
    iTarget(optype::iterBytes)
    iOp1(optype::iterBytes, true);
iEndH

iBeginH(bytes, NextObject, "bytes.next_object")
    iTarget(optype::iterBytes)
    iOp1(optype::iterBytes, true);
iEndH

iBeginH(bytes, Index, "bytes.index")
    iTarget(optype::int64);
    iOp1(optype::iterBytes, true);
iEndH

iBeginH(iterBytes, Begin, "begin")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);
iEndH

iBeginH(iterBytes, End, "end")
    iTarget(optype::iterBytes);
    iOp1(optype::refBytes, true);
iEndH

iBeginH(iterBytes, Incr, "incr")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);
iEndH

iBeginH(iterBytes, IncrBy, "incr_by")
    iTarget(optype::iterBytes);
    iOp1(optype::iterBytes, true);
    iOp2(optype::int64, true);
iEndH

iBeginH(iterBytes, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::iterBytes, true);
    iOp2(optype::iterBytes, true);
iEndH

iBeginH(iterBytes, Deref, "deref")
    iTarget(optype::int8)
    iOp1(optype::iterBytes, true);
iEndH

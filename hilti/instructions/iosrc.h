///
/// \type iosrc<Hilti::IOSrc:*>
///
/// The *iosrc* data type represents a source of external input coming in for
/// processing. It transparently supports a set of different sources
/// (currently only ``libpcap``-based, but in the future potentially other's
/// as well.). Elements of an IOSrc are ``bytes`` objects and come with a
/// timestamp.
///
/// \cproto hlt_iosrc*
///
/// \type ``iter<iosrc>``
///
/// \default An iterator over the elements retrieved from an *iosrc*.
///
/// \cproto hlt_bytes_iter
///

#include "define-instruction.h"

iBeginH(iterIOSource, Begin, "begin")
    iTarget(optype::iterIOSource);
    iOp1(optype::refIOSource, true);
iEndH

iBeginH(iterIOSource, End, "end")
    iTarget(optype::iterIOSource);
    iOp1(optype::refIOSource, true);
iEndH

iBeginH(iterIOSource, Incr, "incr")
    iTarget(optype::iterIOSource);
    iOp1(optype::iterIOSource, true);
iEndH

iBeginH(iterIOSource, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::iterIOSource, true);
    iOp2(optype::iterIOSource, true);
iEndH

iBeginH(iterIOSource, Deref, "deref")
    iTarget(optype::tuple)
    iOp1(optype::iterIOSource, true);
iEndH

iBeginH(ioSource, New, "new")
    iTarget(optype::refIOSource)
    iOp1(optype::typeIOSource, true)
    iOp2(optype::string, true)
iEndH

iBeginH(ioSource, Close, "iosrc.close")
    iOp1(optype::refIOSource, false)
iEndH

iBeginH(ioSource, Read, "iosrc.read")
    iTarget(optype::tuple)
    iOp1(optype::refIOSource, false)
iEndH

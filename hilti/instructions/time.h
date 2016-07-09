/// \type Time
///
/// The ``time`` data type represents specific points in time, with
/// nanosecond resolution. The earliest time that can be represented is the
/// standard UNIX epoch, i.e., Jan 1 1970 UTC.
///
/// Time constants are specified in seconds since the epoch, either as
/// integers (``1295411800``) or as floating poing values
/// (``1295411800.123456789``). In the latter case, there may be at most 9
/// digits for the fractional part.
///
/// A special constant ``Epoch`` is provided as well.
///
/// Note that when operating on time values, behaviour in case of under- and
/// overflow is undefined.
///
/// Internally, ``time`` uses a fixed-point represenation with 32 bit for full
/// seconds, and 32 bit for the fraction of a second.
///
/// \default Epoch
///
/// \ctor time(1295411800.123456789), time(1295411800)
///
/// \cproto hlt_time

#include "define-instruction.h"

iBegin(time, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")

iEnd

iBegin(time, Add, "time.add")
    iTarget(optype::time);
    iOp1(optype::time, true);
    iOp2(optype::interval, true);

    iValidate
    {
    }

    iDoc(R"(    
        Adds interval *op2* to the time *op1*
    )")

iEnd

iBegin(time, AsDouble, "time.as_double")
    iTarget(optype::double_);
    iOp1(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns *op1* in seconds since the epoch, rounded down to the nearest
        value that can be represented as a double.
    )")

iEnd

iBegin(time, FromDouble, "time.from_double")
    iTarget(optype::time);
    iOp1(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the double *op1* into a time value, interpreting it as seconds since the epoch.
    )")
iEnd

iBegin(time, AsInt, "time.as_int")
    iTarget(optype::int64);
    iOp1(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns *op1*' in seconds since the epoch, rounded down to the nearest
        integer.
    )")

iEnd

iBegin(time, Eq, "time.eq")
    iTarget(optype::boolean);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns True if the time represented by *op1* equals that of *op2*.
    )")

iEnd

iBegin(time, Gt, "time.gt")
    iTarget(optype::boolean);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns True if the time represented by *op1* is later than that of
        *op2*.
    )")

iEnd

iBegin(time, Lt, "time.lt")
    iTarget(optype::boolean);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns True if the time represented by *op1* is earlier than that of
        *op2*.
    )")

iEnd

iBegin(time, Leq, "time.leq")
    iTarget(optype::boolean);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns True if the time represented by *op1* is earlier or the same than
        *op2*.
    )")

iEnd

iBegin(time, Geq, "time.geq")
    iTarget(optype::boolean);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns True if the time represented by *op1* is later or the same than
        *op2*.
    )")

iEnd

iBegin(time, Nsecs, "time.nsecs")
    iTarget(optype::int64);
    iOp1(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns the time *op1* as nanoseconds since the epoch.
    )")

iEnd

iBegin(time, SubTime, "time.sub")
    iTarget(optype::interval);
    iOp1(optype::time, true);
    iOp2(optype::time, true);

    iValidate
    {
    }

    iDoc(R"(    
        Subtracts time *op2* from the time *op1*
    )")
iEnd

iBegin(time, SubInterval, "time.sub")
    iTarget(optype::time);
    iOp1(optype::time, true);
    iOp2(optype::interval, true);

    iValidate
    {
    }

    iDoc(R"(    
        Subtracts interval *op2* from the time *op1*
    )")
iEnd

iBegin(time, Wall, "time.wall")
    iTarget(optype::time);

    iValidate
    {
    }

    iDoc(R"(    
        Returns the current wall time (i.e., real-time). Note that the
        resolution of the returned value is OS dependent.
    )")

iEnd

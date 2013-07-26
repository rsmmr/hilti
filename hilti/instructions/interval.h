/// \type Time
///
/// The ``interval`` data type represent a time period of certain length, with
/// nanosecond resolution.
/// 
/// Interval constants are specified in seconds, either as integers (``42``)
/// or as floating poing values (``42.123456789``). In the latter
/// case, there may be at most 9 digits for the fractional part.
/// 
/// Note that when operating on ``interval`` values, behaviour in case of
/// under- and overflow is undefined.
/// 
/// Internally, intervals use a fixed-point represenation with 32 bit for full
/// seconds, and 32 bit for the fraction of a second.
///
/// \default interval(0.0)
///
/// \ctor interval(42.123456789), interval(42)
///
/// \cproto hlt_interval

#include "define-instruction.h"

iBegin(interval, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::interval, true);
    iOp2(optype::interval, true);

    iValidate {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")

iEnd

iBegin(interval, Add, "interval.add")
    iTarget(optype::interval)
    iOp1(optype::interval, true)
    iOp2(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Adds intervals *op1* and *op2*.
    )")

iEnd

iBegin(interval, AsDouble, "interval.as_double")
    iTarget(optype::double_)
    iOp1(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Returns *op1* in seconds, rounded down to the nearest value that can
        be represented as a double.
    )")

iEnd

iBegin(interval, AsInt, "interval.as_int")
    iTarget(optype::int64)
    iOp1(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Returns *op1*' in seconds, rounded down to the nearest integer.
    )")

iEnd

iBegin(interval, Eq, "interval.eq")
    iTarget(optype::boolean)
    iOp1(optype::interval, true)
    iOp2(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Returns True if the interval represented by *op1* equals that of
        *op2*.
    )")

iEnd

iBegin(interval, Gt, "interval.gt")
    iTarget(optype::boolean)
    iOp1(optype::interval, true)
    iOp2(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Returns True if the interval represented by *op1* is larger than that
        of *op2*.
    )")

iEnd

iBegin(interval, Lt, "interval.lt")
    iTarget(optype::boolean)
    iOp1(optype::interval, true)
    iOp2(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Returns True if the interval represented by *op1* is shorter than that
        of *op2*.
    )")

iEnd

iBegin(interval, Mul, "interval.mul")
    iTarget(optype::interval)
    iOp1(optype::interval, true)
    iOp2(optype::int64, true)

    iValidate {
    }

    iDoc(R"(    
        Subtracts interval *op2* to the interval *op1*
    )")

iEnd

iBegin(interval, Nsecs, "interval.nsecs")
    iTarget(optype::int64)
    iOp1(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Returns the interval *op1* as nanoseconds since the epoch.
    )")

iEnd

iBegin(interval, Sub, "interval.sub")
    iTarget(optype::interval)
    iOp1(optype::interval, true)
    iOp2(optype::interval, true)

    iValidate {
    }

    iDoc(R"(    
        Subtracts interval *op2* to the interval *op1*
    )")

iEnd


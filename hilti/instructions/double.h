/// \type Double
///
/// The ``double`` data type represents a 64-bit floating-point numbers.
///
/// \default 0.0
///
/// \ctor 3.14
///
/// \cproto double

#include "define-instruction.h"

iBegin(double_, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEnd

iBegin(double_, Add, "double.add")
    iTarget(optype::double_);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Calculates the sum of the two operands. If the sum overflows the range
        of the double type, the result in undefined.
    )")

iEnd

iBegin(double_, AsInterval, "double.as_interval")
    iTarget(optype::interval);
    iOp1(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the double *op1* into an interval value, interpreting it as
        seconds.
    )")

iEnd

iBegin(double_, AsSInt, "double.as_sint")
    iTarget(optype::integer);
    iOp1(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the double *op1* into a signed integer value, rounding to the
        nearest value.
    )")

iEnd

iBegin(double_, AsTime, "double.as_time")
    iTarget(optype::time);
    iOp1(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the double *op1* into a time value, interpreting it as
        seconds since the epoch.
    )")

iEnd

iBegin(double_, AsUInt, "double.as_uint")
    iTarget(optype::integer);
    iOp1(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Converts the double *op1* into an unsigned integer value, rounding to
        the nearest value.
    )")

iEnd

iBegin(double_, Div, "double.div")
    iTarget(optype::double_);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
        auto cexpr = ast::rtti::tryCast<expression::Constant>(op2);

        if ( cexpr ) {
            auto d = ast::rtti::tryCast<constant::Double>(cexpr->constant())->value();

            if ( d == 0 )
                error(op2, "division by zero");
        }
    }

    iDoc(R"(    
        Divides *op1* by *op2*, flooring the result. Throws
        :exc:`DivisionByZero` if *op2* is zero.
    )")

iEnd

iBegin(double_, Eq, "double.eq")
    iTarget(optype::boolean);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns true iff *op1* equals *op2*.
    )")

iEnd

iBegin(double_, Gt, "double.gt")
    iTarget(optype::boolean);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns true iff *op1* is greater than *op2*.
    )")

iEnd

iBegin(double_, Geq, "double.geq")
    iTarget(optype::boolean);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns true iff *op1* is greater or equal than *op2*.
    )")

iEnd

iBegin(double_, Lt, "double.lt")
    iTarget(optype::boolean);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns true iff *op1* is less than *op2*.
    )")
iEnd

iBegin(double_, Leq, "double.leq")
    iTarget(optype::boolean);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Returns true iff *op1* is less or equal than *op2*.
    )")
iEnd

iBegin(double_, Mod, "double.mod")
    iTarget(optype::double_);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
        auto cexpr = ast::rtti::tryCast<expression::Constant>(op2);

        if ( cexpr ) {
            auto d = ast::rtti::tryCast<constant::Double>(cexpr->constant())->value();

            if ( d == 0 )
                error(op2, "division by zero");
        }
    }

    iDoc(R"(    
        Calculates *op1* modulo *op2*. Throws :exc:`DivisionByZero` if *op2*
        is zero.
    )")

iEnd

iBegin(double_, Mul, "double.mul")
    iTarget(optype::double_);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Multiplies *op1* with *op2*. If the product overflows the range of the
        double type, the result in undefined.
    )")

iEnd

iBegin(double_, PowDouble, "double.pow")
    iTarget(optype::double_);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Raises *op1* to the power *op2*. If the product overflows the range of
        the double type, the result in undefined.
    )")

iEnd

iBegin(double_, Sub, "double.sub")
    iTarget(optype::double_);
    iOp1(optype::double_, true);
    iOp2(optype::double_, true);

    iValidate
    {
    }

    iDoc(R"(    
        Subtracts *op1* one from *op2*. If the difference underflows the range
        of the double type, the result in undefined.
    )")

iEnd

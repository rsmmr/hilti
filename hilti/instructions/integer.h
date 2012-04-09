/// \type Integer
///
/// The *integer* data type represents signed and unsigned integers of a
/// fixed width. The width is specified as parameter to the type, e.g.,
/// ``int<16>`` for a 16-bit integer.
///
/// Note that HILTI has only a single data type representing both signed and
/// unsigned integer values. For instructions where signedness matters, there
/// are always two versions differing only in how they interpret their
/// arguments (e.g., there's ``int.sleq`` and ``int.uleq`` doing
/// lower-or-equal comparision for signed and unsigned operands,
/// respectively.
///
/// If not explictly initialized, integers are set to zero initially.
///
/// \default 0
///
/// \ctor 42, -10
///
/// \cproto An ``int<n>`` is mapped to C integers depending on its width *n*,
/// per the following table:
///
///     ======  =======
///     Width   C type
///     ------  -------
///     1..8    int8_t
///     9..16   int16_t
///     17..32  int32_t
///     33..64  int64_t
///     ======  =======

#include "instructions/define-instruction.h"

iBegin(integer, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::integer, true);
    iOp2(optype::integer, true);

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")
iEnd

iBegin(integer, Incr, "incr")
    iTarget(optype::integer);
    iOp1(optype::integer, true);

    iValidate {
        canCoerceTo(op1, target);
    }

    iDoc(R"(
        Increments *op1* by one.
    )")
iEnd

iBegin(integer, Decr, "decr")
    iTarget(optype::integer);
    iOp1(optype::integer, true);

    iValidate {
        canCoerceTo(op1, target);
    }

    iDoc(R"(
        Decrements *op1* by one.
    )")
iEnd

iBegin(integer, Add, "int.add")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Calculates the sum of the two operands. Operands and target must be of
        same width. The result is calculated modulo 2^{width}.
     )")

iEnd


iBegin(integer, Sub, "int.sub")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Subtracts *op2* from *op1*. Operands and target must be of same width.
        The result is calculated modulo 2^{width}.
     )")

iEnd

iBegin(integer, Div, "int.div")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);

        auto c = ast::as<expression::Constant>(op2);

        if ( c ) {
            auto i = ast::as<constant::Integer>(c->constant());
            assert(i);

            if ( i->value() == 0 )
                error(op2, "Division by zero");
        }
    }

    iDoc(R"(
        Divides *op1* by *op2*, flooring the result. Operands and target must
        be of same width. All integers are treaded as signed. If the product overflows the range of the integer
        type, the result in undefined.  Throws :exc:`DivisionByZero` if *op2*
        is zero.
     )")

iEnd

iBegin(integer, Sleq, "int.sleq")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is lower or equal *op2*, interpreting both as
        *signed* integers.
     )")

iEnd

iBegin(integer, AsSDouble, "int.as_sdouble")
    iTarget(optype::double_)
    iOp1(optype::integer, true)

    iValidate {
    }

    iDoc(R"(
       Converts the signed integer *op1* into a double value.
     )")

 iEnd

iBegin(integer, Pow, "int.pow")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
    }

    iDoc(R"(
        Raise *op1* to the power *op2*. Note that both *op1* and *op2* are
        interpreted as unsigned integers. If the result overflows the target's
        type, the result is undefined.
     )")

iEnd

iBegin(integer, SExt, "int.sext")
    iTarget(optype::integer)
    iOp1(optype::integer, true)

    iValidate {
        auto ty_target = as<type::Integer>(target->type());
        auto ty_op1 = as<type::Integer>(op1->type());

        if ( ty_op1->width() > ty_target->width() )
            error(op1, "width of operand cannot be larger than that of target");
    }

    iDoc(R"(
        Sign-extends *op1* into an integer of the same width as the *target*.
        The width of *op1* must be smaller or equal that of the *target*.
     )")

iEnd

iBegin(integer, Shr, "int.shr")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Shifts *op1* to the right by *op2* bits. The most-signficant bits are
        filled with zeros. If the value of *op2* is larger than the integer
        type has bits, the result is undefined.
     )")

iEnd

iBegin(integer, Mul, "int.mul")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Multiplies *op1* with *op2*. Operands and target must be of same
        width. The result is calculated modulo 2^{width}.
     )")

iEnd

iBegin(integer, Shl, "int.shl")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Shifts *op1* to the left by *op2* bits. The least-signficant bits are
        filled with zeros. If the value of *op2* is larger than the integer
        type has bits, the result is undefined.
     )")

iEnd

iBegin(integer, Ult, "int.ult")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is less than *op2*, interpreting both as
        *unsigned* integers.
     )")

iEnd

iBegin(integer, Uleq, "int.uleq")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is lower or equal *op2*, interpreting both as
        *unsigned* integers.
     )")

iEnd

iBegin(integer, Ashr, "int.ashr")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Arithmetically shifts *op1* to the right by *op2* bits. The most-
        signficant bits are filled with the sign of *op1*. If the value of
        *op2* is larger than the integer type has bits, the result is
        undefined.
     )")

iEnd

iBegin(integer, Mask, "int.mask")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)
    iOp3(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
    }

    iDoc(R"(
        Extracts the bit range from *op2* to *op3* (inclusive) from *op1* and shifts
        them so that they align with the least significant bit in the result.
     )")

iEnd

iBegin(integer, Mod, "int.mod")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);

        auto c = ast::as<expression::Constant>(op2);

        if ( c ) {
            auto i = ast::as<constant::Integer>(c->constant());
            assert(i);

            if ( i->value() == 0 )
                error(op2, "Division by zero");
        }
    }

    iDoc(R"(
        Calculates the remainder of dividing *op1* by *op2*. Operands and
        target must be of same width.  Throws :exc:`DivisionByZero` if *op2*
        is zero.
     )")

iEnd

iBegin(integer, Eq, "int.eq")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* equals *op2*.
     )")

iEnd

iBegin(integer, ZExt, "int.zext")
    iTarget(optype::integer)
    iOp1(optype::integer, true)

    iValidate {
        auto ty_target = as<type::Integer>(target->type());
        auto ty_op1 = as<type::Integer>(op1->type());

        if ( ty_op1->width() > ty_target->width() )
            error(op1, "width of operand cannot be larger than that of target");
    }

    iDoc(R"(
        Zero-extends *op1* into an integer of the same width as the *target*.
        The width of *op1* must be smaller or equal that of the *target*.
     )")

iEnd

iBegin(integer, AsTime, "int.as_time")
   iTarget(optype::time)
     iOp1(optype::integer, true)

     iValidate {
     }

     iDoc(R"(
         Converts the integer *op1* into a time value, interpreting it as
         seconds since the epoch.
      )")
iEnd

iBegin(integer, Slt, "int.slt")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is less than *op2*, interpreting both as
        *signed* integers.
     )")

iEnd

iBegin(integer, Trunc, "int.trunc")
    iTarget(optype::integer)
    iOp1(optype::integer, true)

    iValidate {
        auto ty_target = as<type::Integer>(target->type());
        auto ty_op1 = as<type::Integer>(op1->type());

        if ( ty_op1->width() < ty_target->width() )
            error(op1, "width of operand cannot be smaller than that of target");
    }

    iDoc(R"(
        Bit-truncates *op1* into an integer of the same width as the *target*.
        The width of *op1* must be larger or equal that of the *target*.
     )")

iEnd

iBegin(integer, Sqeq, "int.sgeq")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is greater or equal *op2*, interpreting both as
        *signed* integers.
     )")

iEnd

iBegin(integer, AsUDouble, "int.as_udouble")
    iTarget(optype::double_)
    iOp1(optype::integer, true)

    iValidate {
    }

    iDoc(R"(
        Converts the unsigned integer *op1* into a double value.
     )")

iEnd

iBegin(integer, Or, "int.or")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Calculates the binary *or* of the two operands. Operands and target
        must be of same width.
     )")

iEnd

iBegin(integer, Ugeq, "int.ugeq")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is greater or equal *op2*, interpreting both as
        *unsigned* integers.
     )")

iEnd

iBegin(integer, Sgt, "int.sgt")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is greater than *op2*, interpreting both as
        *signed* integers.
     )")

iEnd

iBegin(integer, AsInterval, "int.as_interval")
    iTarget(optype::interval)
    iOp1(optype::integer, true)

    iValidate {
    }

    iDoc(R"(
        Converts the integer *op1* into an interval value, interpreting it as
        seconds.
     )")
iEnd

iBegin(integer, Xor, "int.xor")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Calculates the binary *xor* of the two operands. Operands and target
        must be of same width.
     )")

iEnd

iBegin(integer, And, "int.and")
    iTarget(optype::integer)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTo(op1, target);
        canCoerceTo(op2, target);
    }

    iDoc(R"(
        Calculates the binary *and* of the two operands. Operands and target
        must be of same width.
     )")

iEnd

iBegin(integer, Ugt, "int.ugt")
    iTarget(optype::boolean)
    iOp1(optype::integer, true)
    iOp2(optype::integer, true)

    iValidate {
        canCoerceTypes(op1, op2);
    }

    iDoc(R"(
        Returns true iff *op1* is greater than *op2*, interpreting both as
        *unsigned* integers.
     )")

iEnd

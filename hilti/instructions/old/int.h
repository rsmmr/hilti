iBegin(int, Add, "int.add")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Calculates the sum of the two operands. Operands and target must be of
        same width. The result is calculated modulo 2^{width}.
    )")

iEnd

iBegin(int, And, "int.and")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Calculates the binary *and* of the two operands. Operands and target
        must be of same width.
    )")

iEnd

iBegin(int, AsDouble, "int.as_interval")
    iTarget(optype::interval)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::interval>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Converts the integer *op1* into an interval value, interpreting it as
        seconds.
    )")

iEnd

iBegin(int, SInt, "int.as_sdouble")
    iTarget(optype::double)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Converts the signed integer *op1* into a double value.
    )")

iEnd

iBegin(int, AsTime, "int.as_time")
    iTarget(optype::time)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::time>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Converts the integer *op1* into a time value, interpreting it as
        seconds since the epoch.
    )")

iEnd

iBegin(int, UInt, "int.as_udouble")
    iTarget(optype::double)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::double>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Converts the unsigned integer *op1* into a double value.
    )")

iEnd

iBegin(int, Ashr, "int.ashr")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Arithmetically shifts *op1* to the right by *op2* bits. The most-
        signficant bits are filled with the sign of *op1*. If the value of
        *op2* is larger than the integer type has bits, the result is
        undefined.
    )")

iEnd

iBegin(int, Div, "int.div")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Divides *op1* by *op2*, flooring the result. Operands and target must
        be of same width.  If the product overflows the range of the integer
        type, the result in undefined.  Throws :exc:`DivisionByZero` if *op2*
        is zero.
    )")

iEnd

iBegin(int, Eq, "int.eq")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* equals *op2*.
    )")

iEnd

iBegin(int, Xor, "int.mask")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int\ <64>, trueX)
    iOp3(optype::int\ <64>, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int\ <64>>(op2->type());
        auto ty_op3 = as<type::int\ <64>>(op3->type());

    }

    iDoc(R"(    
        Extracts the bits *op2* to *op3* (inclusive) from *op1* and shifts
        them so that they align with the least significant bit in the result.
    )")

iEnd

iBegin(int, Mod, "int.mod")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Calculates the remainder of dividing *op1* by *op2*. Operands and
        target must be of same width.  Throws :exc:`DivisionByZero` if *op2*
        is zero.
    )")

iEnd

iBegin(int, Mul, "int.mul")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Multiplies *op1* with *op2*. Operands and target must be of same
        width. The result is calculated modulo 2^{width}.
    )")

iEnd

iBegin(int, Or, "int.or")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Calculates the binary *or* of the two operands. Operands and target
        must be of same width.
    )")

iEnd

iBegin(int, Pow, "int.pow")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Raise *op1* to the power *op2*. Note that both *op1* and *op2* are
        interpreted as unsigned integers. If the result overflows the target's
        type, the result is undefined.
    )")

iEnd

iBegin(int, SExt, "int.sext")
    iTarget(optype::int)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Sign-extends *op1* into an integer of the same width as the *target*.
        The width of *op1* must be smaller or equal that of the *target*.
    )")

iEnd

iBegin(int, Sqeq, "int.sgeq")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is greater or equal *op2*, interpreting both as
        *signed* integers.
    )")

iEnd

iBegin(int, Sgt, "int.sgt")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is greater than *op2*, interpreting both as
        *signed* integers.
    )")

iEnd

iBegin(int, Shl, "int.shl")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Shifts *op1* to the left by *op2* bits. The least-signficant bits are
        filled with zeros. If the value of *op2* is larger than the integer
        type has bits, the result is undefined.
    )")

iEnd

iBegin(int, Shr, "int.shr")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Shifts *op1* to the right by *op2* bits. The most-signficant bits are
        filled with zeros. If the value of *op2* is larger than the integer
        type has bits, the result is undefined.
    )")

iEnd

iBegin(int, Sleq, "int.sleq")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is lower or equal *op2*, interpreting both as
        *signed* integers.
    )")

iEnd

iBegin(int, Slt, "int.slt")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is less than *op2*, interpreting both as
        *signed* integers.
    )")

iEnd

iBegin(int, Sub, "int.sub")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Subtracts *op2* from *op1*. Operands and target must be of same width.
        The result is calculated modulo 2^{width}.
    )")

iEnd

iBegin(int, Trunc, "int.trunc")
    iTarget(optype::int)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Bit-truncates *op1* into an integer of the same width as the *target*.
        The width of *op1* must be larger or equal that of the *target*.
    )")

iEnd

iBegin(int, Sqeq, "int.ugeq")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is greater or equal *op2*, interpreting both as
        *unsigned* integers.
    )")

iEnd

iBegin(int, Ugt, "int.ugt")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is greater than *op2*, interpreting both as
        *unsigned* integers.
    )")

iEnd

iBegin(int, Sleq, "int.uleq")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is lower or equal *op2*, interpreting both as
        *unsigned* integers.
    )")

iEnd

iBegin(int, Ult, "int.ult")
    iTarget(optype::bool)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::bool>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Returns true iff *op1* is less than *op2*, interpreting both as
        *unsigned* integers.
    )")

iEnd

iBegin(int, Xor, "int.xor")
    iTarget(optype::int)
    iOp1(optype::int, trueX)
    iOp2(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());
        auto ty_op2 = as<type::int>(op2->type());

    }

    iDoc(R"(    
        Calculates the binary *xor* of the two operands. Operands and target
        must be of same width.
    )")

iEnd

iBegin(int, ZExt, "int.zext")
    iTarget(optype::int)
    iOp1(optype::int, trueX)

    iValidate {
        auto ty_target = as<type::int>(target->type());
        auto ty_op1 = as<type::int>(op1->type());

    }

    iDoc(R"(    
        Zero-extends *op1* into an integer of the same width as the *target*.
        The width of *op1* must be smaller or equal that of the *target*.
    )")

iEnd


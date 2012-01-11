
/// \type Integer
///
/// Bla bla.
///
/// \constant  42, -10
///
/// \cproto int8_t,int16_t,int32_t,int64_t,

iBegin(integer, Add, "int.add")
    iTarget(type::Integer)
    iOp1(type::Integer, true)
    iOp2(type::Integer, true)

    iValidate {
        auto tyTarget = as<type::Integer>(target->type());
        auto tyOp1 = as<type::Integer>(op1->type());
        auto tyOp2 = as<type::Integer>(op2->type());

        if ( ! op1->canCoerceTo(tyTarget) )
            error(op1, "op1 does not match with width of target");

        if ( ! op2->canCoerceTo(tyTarget) )
            error(op1, "op2 does not match with width of target");
    }

    iDoc("Calculates the sum of the two operands. Operands and target must be of same width. The result is calculated modulo 2^{width}.")
iEnd

iBegin(integer, Sub, "int.sub")
    iTarget(type::Integer)
    iOp1(type::Integer, true)
    iOp2(type::Integer, true)

    iValidate {
        auto tyTarget = as<type::Integer>(target->type());
        auto tyOp1 = as<type::Integer>(op1->type());
        auto tyOp2 = as<type::Integer>(op2->type());

        if ( ! op1->canCoerceTo(tyTarget) )
            error(op1, "op1 does not match with width of target");

        if ( ! op2->canCoerceTo(tyTarget) )
            error(op1, "op2 does not match with width of target");
    }

    iDoc("Subtracts *op2* from *op1*. Operands and target must be of same width. The result is calculated modulo 2^{width}.")
iEnd

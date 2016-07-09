/// \type Booleans
///
/// The *bool* data type represents boolean values.
///
/// \default False
///
/// \ctor True, False
///
/// \cproto int8_t

#include "define-instruction.h"

iBegin(boolean, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::boolean, true);
    iOp2(optype::boolean, true);

    iValidate
    {
    }

    iDoc(R"(
        Returns true if *op1* is equal to *op2*.
    )")

iEnd

iBegin(boolean, And, "bool.and")
    iTarget(optype::boolean);
    iOp1(optype::boolean, true);
    iOp2(optype::boolean, true);

    iValidate
    {
    }

    iDoc(R"(
        Computes the logical 'and' of *op1* and *op2*.
    )")

iEnd

iBegin(boolean, Not, "bool.not")
    iTarget(optype::boolean);
    iOp1(optype::boolean, true);

    iValidate
    {
    }

    iDoc(R"(    
        Computes the logical 'not' of *op1*.
    )")

iEnd

iBegin(boolean, Or, "bool.or")
    iTarget(optype::boolean);
    iOp1(optype::boolean, true);
    iOp2(optype::boolean, true);

    iValidate
    {
    }

    iDoc(R"(    
        Computes the logical 'or' of *op1* and *op2*.
    )")

iEnd

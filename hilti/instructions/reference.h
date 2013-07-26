
#include "define-instruction.h"

iBegin(reference, AsBool, "ref.as_bool")
    iTarget(optype::boolean)
    iOp1(optype::refAny, true)

    iValidate {
    }

    iDoc(R"(    
        Converts *op1* into a boolean. The boolean's value will be True if
        *op1* references a valid object, and False otherwise.
    )")

iEnd


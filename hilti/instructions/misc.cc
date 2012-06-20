
#include "instructions/define-instruction.h"

#include "misc.h"
#include "module.h"
#include "builder/nodes.h"

iBeginCC(Misc)
    iValidateCC(Select) {
        auto ty_op1 = as<type::ValueType>(op1->type());
        auto ty_op2 = as<type::Label>(op2->type());
        auto ty_op3 = as<type::Tuple>(op3->type());

        canCoerceTo(op2, target->type());
        canCoerceTo(op3, target->type());
    }

    iDocCC(Select, R"(
        Returns *op2* if *op1* is True, and *op3* otherwise.
    )")
iEndCC

iBeginCC(Misc)
    iValidateCC(Nop) {
    }

    iDocCC(Nop, R"(
        Evaluates into nothing.
    )")
iEndCC

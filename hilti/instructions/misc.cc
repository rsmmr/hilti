
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
    iValidateCC(SelectValue) {
        auto ty_op1 = as<type::ValueType>(op1->type());
        auto ty_op2 = as<type::Tuple>(op2->type());

        if ( op3 )
            canCoerceTo(op3, target->type());

        for ( auto t : ty_op2->typeList() ) {
            auto tt = ast::tryCast<type::Tuple>(t);
            if ( ! tt || tt->typeList().size() != 2 ) {
                error(ty_op2, "operand 2 must a tuple of 2-tuples");
                return;
            }

            auto l = tt->typeList();
            auto i = l.begin();
            auto t1 = *i++;
            auto t2 = *i++;

            canCoerceTo(op1, t1);
            canCoerceTo(t2, target->type());
        }
    }

    iDocCC(SelectValue, R"(
        Matches *op1* against the first elements of set of 2-tuples *op2*. If a match is
        found, returns the second element of the corresponding 2-tuple. If multiple matches
        are found, one is returned but it's undefined which one. If no match is found, returns *op3*
        if given, or throws a ValueError otherwise.
    )")
iEndCC

iBeginCC(Misc)
    iValidateCC(Nop) {
    }

    iDocCC(Nop, R"(
        Evaluates into nothing.
    )")
iEndCC

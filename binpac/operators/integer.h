
#include "operator.h"

namespace operator_ {
namespace integer {

opBegin(integer::Plus)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Adds two integers.")

    opResult() {
        auto op = exprs.begin();
        op1 = *op++;
        op2 = *op++;

        int w1 = ast::checkedCast<type::Integer>(op1->type())->width();
        int w2 = ast::checkedCast<type::Integer>(op2->type())->width();

        return std::make_shared<type::Integer>(max(w1, w2));
    }
opEnd

opBegin(integer::Minus)
    opOp1(std::make_shared<type::Integer>())
    opOp2(std::make_shared<type::Integer>())

    opDoc("Subtracts two integers.")

    opResult() {
        auto op = exprs.begin();
        op1 = *op++;
        op2 = *op++;

        int w1 = ast::checkedCast<type::Integer>(op1->type())->width();
        int w2 = ast::checkedCast<type::Integer>(op2->type())->width();

        return std::make_shared<type::Integer>(max(w1, w2));
    }
opEnd








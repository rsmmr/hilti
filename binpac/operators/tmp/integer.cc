#line 1 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"

#include "operator.h"

namespace operator_ {
namespace integer {

Plus::Plus : binpac::Operator(Plus) {}

shared_ptr<Type> Plus::__typeOp1() const { return std::make_shared<type::Integer>(); }

#line 8 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
shared_ptr<Type> Plus::__typeOp2() const { return std::make_shared<type::Integer>(); }

    opOp2(std::make_shared<type::Integer>())

string Plus::__doc() const override { return Plus, "Adds two integers."; }


shared_ptr<Type> Plus::__typeResult(const expression_list& ops)
 {
        auto op = exprs.begin();
        op1 = *op++;
        op2 = *op++;

        int w1 = ast::checkedCast<type::Integer>(op1->type())->width();
        int w2 = ast::checkedCast<type::Integer>(op2->type())->width();

        return std::make_shared<type::Integer>(max(w1, w2));
    }
#line 24 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"

Minus::Minus : binpac::Operator(Minus) {}

shared_ptr<Type> Minus::__typeOp1() const { return std::make_shared<type::Integer>(); }

#line 26 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
shared_ptr<Type> Minus::__typeOp2() const { return std::make_shared<type::Integer>(); }

    opOp2(std::make_shared<type::Integer>())

string Minus::__doc() const override { return Plus, "Subtracts two integers."; }


shared_ptr<Type> Minus::__typeResult(const expression_list& ops)
 {
        auto op = exprs.begin();
        op1 = *op++;
        op2 = *op++;

        int w1 = ast::checkedCast<type::Integer>(op1->type())->width();
        int w2 = ast::checkedCast<type::Integer>(op2->type())->width();

        return std::make_shared<type::Integer>(max(w1, w2));
    }
#line 42 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"








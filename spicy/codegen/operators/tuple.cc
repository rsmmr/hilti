
#include "cg-operator-common.h"
#include <spicy/autogen/operators/tuple.h>

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(constant::Tuple* t)
{
    hilti::builder::tuple::element_list elems;

    for ( auto e : t->value() ) {
        if ( ! e->usesTryAttribute() ) {
            elems.push_back(cg()->hiltiExpression(e));
            continue;
        }

        // Wrap it into an exception handler that return an unset optional if
        // an SpicyHilti::AttributeNotSet is thrown.

        auto tmp = cg()->builder()->addTmp("opt", cg()->hiltiTypeOptional(e->type()));

        cg()->builder()->beginTryCatch();

        auto he = cg()->hiltiConstantOptional(e);
        cg()->builder()->addInstruction(tmp, ::hilti::instruction::operator_::Assign, he);

        cg()->builder()->pushCatch(hilti::builder::reference::type(
                                       hilti::builder::type::byName("SpicyHilti::AttributeNotSet")),
                                   hilti::builder::id::node("e"));

        // Nothing to do, default of tmp is right.

        cg()->builder()->popCatch();

        cg()->builder()->endTryCatch();

        elems.push_back(tmp);
    }

    auto result = hilti::builder::tuple::create(elems, t->location());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::tuple::CoerceTuple* t)
{
    auto op1 = cg()->hiltiExpression(t->op1());

    // For now we cheat here and rely on HILTI to do the coercion right.
    // Otherwise, we'd have to split the tuple apart, coerce each element
    // individually, and then put it back together; something which is
    // probably always unnecessary. However, I'm not sure we'll eventually
    // get around that ...

    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::tuple::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::tuple::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::tuple::Index* i)
{
    auto tuple = ast::rtti::checkedCast<type::Tuple>(i->op1()->type());
    auto types = tuple->typeList();

    auto idx = ast::rtti::checkedCast<expression::Constant>(i->op2());
    auto const_ = ast::rtti::checkedCast<constant::Integer>(idx->constant());

    auto t = types.begin();
    std::advance(t, const_->value());

    auto result = builder()->addTmp("elem", cg()->hiltiType(*t));
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::tuple::Index, op1, op2);
    setResult(result);
}

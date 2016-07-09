
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"
#include "hilti/autogen/instructions.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::Misc::Select* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());
    auto op3 = cg()->llvmValue(i->op3(), i->target()->type());

    auto result = cg()->builder()->CreateSelect(op1, op2, op3);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::Misc::SelectValue* i)
{
    auto op3 = i->op3() ? cg()->llvmValue(i->op3(), i->target()->type()) : nullptr;

    auto a1 = ast::rtti::tryCast<expression::Constant>(i->op2());
    auto a2 = ast::rtti::tryCast<constant::Tuple>(a1->constant());

    std::list<std::pair<llvm::Value*, llvm::Value*>> alts;

    for ( auto c : a2->value() ) {
        auto c1 = ast::rtti::tryCast<expression::Constant>(c);
        auto c2 = ast::rtti::tryCast<constant::Tuple>(c1->constant());

        auto v = c2->value();
        auto j = v.begin();
        auto val = cg()->llvmValue((*j++)->coerceTo(i->op1()->type()));
        auto result = cg()->llvmValue(*j++);

        alts.push_back(std::make_pair(val, result));
    }

    // We build an if-else chain using the type's standard comparision
    // operator.

    auto done = cg()->newBuilder("select-done");
    auto cmp = cg()->makeLocal("select-cmp", builder::boolean::type());

    for ( auto a : alts ) {
        auto choose_value = cg()->pushBuilder("select-choose");
        cg()->llvmStore(i, a.second);
        cg()->builder()->CreateBr(done->GetInsertBlock());
        cg()->popBuilder();

        cg()->llvmInstruction(cmp, instruction::operator_::Equal, i->op1(),
                              builder::codegen::create(i->op1()->type(), a.first));
        auto match = cg()->builder()->CreateICmpEQ(cg()->llvmConstInt(1, 1), cg()->llvmValue(cmp));
        auto next_block = cg()->newBuilder("select-chain");
        cg()->builder()->CreateCondBr(match, choose_value->GetInsertBlock(),
                                      next_block->GetInsertBlock());
        cg()->pushBuilder(next_block);
    }

    // Do the default.
    if ( ! op3 )
        cg()->llvmRaiseException("Hilti::ValueError", i->location());

    else {
        cg()->llvmStore(i, op3);
        cg()->builder()->CreateBr(done->GetInsertBlock());
    }

    cg()->pushBuilder(done);

    // Leave builder on stack.
}

void StatementBuilder::visit(statement::instruction::Misc::Nop* i)
{
    // Nothing to do.
}

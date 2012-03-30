
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::tuple::Equal* i)
{
    auto ttype = ast::as<type::Tuple>(i->op1()->type());

    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto block_no = cg()->newBuilder("tuple-nomatch");
    auto block_cont = cg()->newBuilder("tuple-cont");
    auto block_next = cg()->newBuilder("tuple-match-0");

    auto cmp = cg()->llvmAddTmp("elem-cmp", cg()->llvmTypeInt(1), nullptr, true);
    auto o0 = builder::codegen::create(builder::boolean::type(), cmp);

    cg()->llvmCreateBr(block_next);

    int n = 0;

    auto equal = std::make_shared<ID>("equal");

    for ( auto t : ttype->typeList() ){
        cg()->pushBuilder(block_next);

        auto e1 = cg()->llvmExtractValue(op1, n);
        auto e2 = cg()->llvmExtractValue(op2, n);

        // Build our own "equal" statement.
        auto o1 = builder::codegen::create(t, e1);
        auto o2 = builder::codegen::create(t, e2);

        auto instrs = theInstructionRegistry->getMatching(equal, { o0, o1, o2 });
        assert(instrs.size() == 1);
        auto instr = instrs.front();

        auto eq = (*instr->factory())(instr, { o0, o1, o2 }, Location::None);

        StatementBuilder(cg()).llvmStatement(eq);

        // Result is now in cmp.

        block_next = cg()->newBuilder(::util::fmt("tuple-match-%d",(n+1)));
        cg()->llvmCreateCondBr(builder()->CreateLoad(cmp), block_next, block_no);
        cg()->popBuilder();

        ++n;
    }

    cg()->pushBuilder(block_next);
    cg()->llvmStore(i, cg()->llvmConstInt(1, 1));
    cg()->llvmCreateBr(block_cont);
    cg()->popBuilder();

    cg()->pushBuilder(block_no);
    cg()->llvmStore(i, cg()->llvmConstInt(0, 1));
    cg()->llvmCreateBr(block_cont);
    cg()->popBuilder();

    cg()->pushBuilder(block_cont); // Leave on stack.
}

void StatementBuilder::visit(statement::instruction::tuple::Index* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto c = ast::as<expression::Constant>(i->op2())->constant();
    assert(c);

    auto idx = ast::as<constant::Integer>(c)->value();
    auto result = cg()->llvmTupleElement(i->op1()->type(), op1, idx, true);

    cg()->llvmStore(i, result);
}


#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::vector::New* i)
{
    auto etype = ast::as<type::Vector>(typedType(i->op1()))->argType();
    auto op1 = builder::type::create(etype);

    shared_ptr<Expression> op2 = i->op2();

    if ( ! op2 ) {
        auto tmgr = builder::timer_mgr::type();
        auto rtmgr = builder::reference::type(tmgr);
        auto n = cg()->llvmConstNull(cg()->llvmTypePtr());

        op2 = builder::codegen::create(rtmgr, n);
    }

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::vector_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::vector::Get* i)
{
    auto etype = ast::as<type::Vector>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(builder::integer::type(64));

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);

    auto voidp = cg()->llvmCall("hlt::vector_get", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmCctor(voidp, etype, true);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::vector::PushBack* i)
{
    auto etype = ast::as<type::Vector>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::vector_push_back", args);
}

void StatementBuilder::visit(statement::instruction::vector::Reserve* i)
{
    auto op2 = i->op2()->coerceTo(builder::integer::type(64));

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::X", args);
}

void StatementBuilder::visit(statement::instruction::vector::Set* i)
{
    auto etype = ast::as<type::Vector>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(builder::integer::type(64));
    auto op3 = i->op3()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    args.push_back(op3);
    cg()->llvmCall("hlt::vector_set", args);
}

void StatementBuilder::visit(statement::instruction::vector::Size* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::vector_size", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::vector::Timeout* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());
    cg()->llvmCall("hlt::vector_timeout", args);
}

void StatementBuilder::visit(statement::instruction::iterVector::Begin* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::vector_begin", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterVector::End* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::vector_end", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterVector::Incr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::vector_iter_incr", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterVector::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::vector_iter_eq", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterVector::Deref* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::vector_iter_deref", args);

    cg()->llvmStore(i, result);
}

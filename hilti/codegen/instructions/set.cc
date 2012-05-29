
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::set::New* i)
{
    auto etype = ast::as<type::Set>(typedType(i->op1()))->argType();
    auto op1 = builder::type::create(etype);

    shared_ptr<Expression> op2 = i->op2();

    if ( ! op2 ) {
        auto tmgr = builder::timer_mgr::type();
        auto rtmgr = builder::reference::type(tmgr);
        auto n = cg()->llvmConstNull(cg()->llvmType(rtmgr));

        op2 = builder::codegen::create(rtmgr, n);
    }

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::set_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::set::Clear* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::set_clear", args);
}

void StatementBuilder::visit(statement::instruction::set::Exists* i)
{
    auto etype = ast::as<type::Set>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::set_exits", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::set::Insert* i)
{
    auto etype = ast::as<type::Set>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::set_insert", args);
}

void StatementBuilder::visit(statement::instruction::set::Remove* i)
{
    auto etype = ast::as<type::Set>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::set_remove", args);
}

void StatementBuilder::visit(statement::instruction::set::Size* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::set_size", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::set::Timeout* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());
    cg()->llvmCall("hlt::set_timeout", args);
}

void StatementBuilder::visit(statement::instruction::iterSet::Begin* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::set_begin", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterSet::End* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::set_end", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterSet::Incr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_set_incr", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterSet::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_set_eq", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterSet::Deref* i)
{
    auto etype = ast::as<type::Set>(iteratedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto voidp = cg()->llvmCall("hlt::iterator_set_deref", args);
    auto casted = cg()->builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));

    cg()->llvmStore(i, cg()->builder()->CreateLoad(casted));
}

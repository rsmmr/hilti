
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::list::New* i)
{
    auto etype = ast::as<type::List>(typedType(i->op1()))->argType();
    auto op1 = builder::type::create(etype);

    shared_ptr<Expression> op2 = i->op2(); // sic

    if ( ! op2 ) {
        auto tmgr = builder::timer_mgr::type();
        auto rtmgr = builder::reference::type(tmgr);
        auto n = cg()->llvmConstNull(cg()->llvmType(rtmgr));

        op2 = builder::codegen::create(rtmgr, n);
    }

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::list_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::list::Back* i)
{
    auto etype = ast::as<type::List>(referencedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto voidp = cg()->llvmCall("hlt::list_back", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::list::Erase* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::list_erase", args);
}

void StatementBuilder::visit(statement::instruction::list::Front* i)
{
    auto etype = ast::as<type::List>(referencedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto voidp = cg()->llvmCall("hlt::list_front", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::list::Insert* i)
{
    auto etype = ast::as<type::List>(iteratedType(i->op2()))->argType();
    auto op1 = i->op1()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(i->op2());
    cg()->llvmCall("hlt::list_insert", args);
}

void StatementBuilder::visit(statement::instruction::list::PopBack* i)
{
    auto etype = ast::as<type::List>(referencedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto voidp = cg()->llvmCall("hlt::list_back", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmCall("hlt::list_pop_back", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::list::PopFront* i)
{
    auto etype = ast::as<type::List>(referencedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto voidp = cg()->llvmCall("hlt::list_front", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmCall("hlt::list_pop_front", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::list::PushBack* i)
{
    auto etype = ast::as<type::List>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::list_push_back", args);
}

void StatementBuilder::visit(statement::instruction::list::PushFront* i)
{
    auto etype = ast::as<type::List>(referencedType(i->op1()))->argType();
    auto op2 = i->op2()->coerceTo(etype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::list_push_front", args);
}

void StatementBuilder::visit(statement::instruction::list::Append* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::list_append", args);
}

void StatementBuilder::visit(statement::instruction::list::Size* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::list_size", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::list::Timeout* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());
    cg()->llvmCall("hlt::list_timeout", args);
}

void StatementBuilder::visit(statement::instruction::iterList::Begin* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::list_begin", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterList::End* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::list_end", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterList::Incr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_list_incr", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterList::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_list_eq", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterList::Deref* i)
{
    auto etype = ast::as<type::List>(iteratedType(i->op1()))->argType();

    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto voidp = cg()->llvmCall("hlt::iterator_list_deref", args);
    auto casted = cg()->builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(etype)));

    cg()->llvmStore(i, cg()->builder()->CreateLoad(casted));
}


#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::map::New* i)
{
    auto ktype = ast::as<type::Map>(typedType(i->op1()))->keyType();
    auto vtype = ast::as<type::Map>(typedType(i->op1()))->valueType();

    auto op1 = builder::type::create(ktype);
    auto op2 = builder::type::create(vtype);

    shared_ptr<Expression> op3 = i->op2(); // sic

    if ( ! op3 ) {
        auto tmgr = builder::timer_mgr::type();
        auto rtmgr = builder::reference::type(tmgr);
        auto n = cg()->llvmConstNull(cg()->llvmType(rtmgr));

        op3 = builder::codegen::create(rtmgr, n);
    }

    CodeGen::expr_list args;
    args.push_back(op1);
    args.push_back(op2);
    args.push_back(op3);
    auto result = cg()->llvmCall("hlt::map_new", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::map::Clear* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    cg()->llvmCall("hlt::map_clear", args);
}

void StatementBuilder::visit(statement::instruction::map::Default* i)
{
    auto vtype = ast::as<type::Map>(referencedType(i->op1()))->valueType();
    auto op2 = i->op2();

    auto rtype = ast::tryCast<type::Reference>(i->op2()->type());

    if ( ! (rtype && ast::isA<type::Callable>(rtype->argType())) )
        op2 = op2->coerceTo(vtype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::map_default", args);
}

void StatementBuilder::visit(statement::instruction::map::Exists* i)
{
    auto ktype = ast::as<type::Map>(referencedType(i->op1()))->keyType();
    auto op2 = i->op2()->coerceTo(ktype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    auto result = cg()->llvmCall("hlt::map_exists", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::map::Get* i)
{
    auto ktype = ast::as<type::Map>(referencedType(i->op1()))->keyType();
    auto vtype = ast::as<type::Map>(referencedType(i->op1()))->valueType();
    auto op2 = i->op2()->coerceTo(ktype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);

    auto voidp = cg()->llvmCall("hlt::map_get", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(vtype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::map::GetDefault* i)
{
    auto ktype = ast::as<type::Map>(referencedType(i->op1()))->keyType();
    auto vtype = ast::as<type::Map>(referencedType(i->op1()))->valueType();
    auto op2 = i->op2()->coerceTo(ktype);
    auto op3 = i->op3()->coerceTo(vtype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    args.push_back(op3);

    auto voidp = cg()->llvmCall("hlt::map_get_default", args);
    auto casted = builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(vtype)));
    auto result = builder()->CreateLoad(casted);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::map::Insert* i)
{
    auto ktype = ast::as<type::Map>(referencedType(i->op1()))->keyType();
    auto vtype = ast::as<type::Map>(referencedType(i->op1()))->valueType();
    auto op2 = i->op2()->coerceTo(ktype);
    auto op3 = i->op3()->coerceTo(vtype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    args.push_back(op3);

    cg()->llvmCall("hlt::map_insert", args);
}

void StatementBuilder::visit(statement::instruction::map::Remove* i)
{
    auto ktype = ast::as<type::Map>(referencedType(i->op1()))->keyType();
    auto op2 = i->op2()->coerceTo(ktype);

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(op2);
    cg()->llvmCall("hlt::map_remove", args);
}

void StatementBuilder::visit(statement::instruction::map::Size* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::map_size", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::map::Timeout* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    args.push_back(i->op3());
    cg()->llvmCall("hlt::map_timeout", args);
}

void StatementBuilder::visit(statement::instruction::iterMap::Begin* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::map_begin", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterMap::End* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::map_end", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterMap::Incr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_map_incr", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterMap::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_map_eq", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterMap::Deref* i)
{
    CodeGen::expr_list args;
    args.push_back(builder::type::create(i->target()->type()));
    args.push_back(i->op1());
    auto voidp = cg()->llvmCall("hlt::iterator_map_deref", args);
    auto casted = cg()->builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(i->target()->type())));

    cg()->llvmStore(i, cg()->builder()->CreateLoad(casted));
}

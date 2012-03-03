
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

static string _opToStr(shared_ptr<Expression> op)
{
    // The validator makes sure that we can't get expression here which is in
    // fact not a string constant.
    auto cexpr = ast::as<expression::Constant>(op);
    assert(cexpr);
    auto cval = ast::as<constant::String>(cexpr->constant());
    assert(cval);

    return cval->value();
}

void StatementBuilder::visit(statement::instruction::struct_::New* i)
{
    auto stype = ast::as<type::Struct>(typedType(i->op1()));
    auto result = cg()->llvmStructNew(stype);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::struct_::Get* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    auto result = cg()->llvmStructGet(stype, op1, nullptr, nullptr, nullptr, i->location());
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::struct_::GetDefault* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    auto op3 = cg()->llvmValue(i->op3());
    auto result = cg()->llvmStructGet(stype, op1, _opToStr(i->op2()), op3, nullptr, i->location());
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::struct_::IsSet* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    auto result = cg()->llvmStructIsSet(stype, op1, _opToStr(i->op2()));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::struct_::Set* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    auto op3 = cg()->llvmValue(i->op3());
}

void StatementBuilder::visit(statement::instruction::struct_::Unset* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    cg()->llvmStructUnset(stype, op1, _opToStr(i->op2()));
}


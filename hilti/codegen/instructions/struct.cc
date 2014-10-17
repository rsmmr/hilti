
#include <hilti/hilti-intern.h>

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
    auto result = cg()->llvmStructNew(stype, false);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::struct_::Get* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    auto result = cg()->llvmStructGet(stype, op1, _opToStr(i->op2()), nullptr, nullptr, i->location());
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::struct_::GetDefault* i)
{
    auto fname = _opToStr(i->op2());
    auto stype = referencedType(i->op1());
    auto field =  ast::checkedCast<type::Struct>(stype)->lookup(std::make_shared<ID>(fname));
    assert(field);

    auto op1 = cg()->llvmValue(i->op1());
    auto op3 = cg()->llvmValue(i->op3(), field->type());
    auto result = cg()->llvmStructGet(stype, op1, fname,
#if 1
                                      [&] (CodeGen* cg) -> llvm::Value* {
                                          auto clone = cg->llvmCreateAlloca(op3->getType());
                                          auto orig  = cg->llvmCreateAlloca(op3->getType());
                                          cg->llvmCreateStore(op3, orig);

                                          auto src = cg->builder()->CreateBitCast(orig, cg->llvmTypePtr());
                                          auto dst = cg->builder()->CreateBitCast(clone, cg->llvmTypePtr());
                                          CodeGen::value_list args = { dst, cg->llvmRtti(i->op3()->type()), src };
                                          cg->llvmCallC("hlt_clone_deep", args, true, false);
                                          cg->llvmDtor(clone, field->type(), true, "struct.get-default");
                                          return cg->builder()->CreateLoad(clone);
                                      },
#else
                                      [&] (CodeGen* cg) -> llvm::Value* { return op3; },
#endif

                                      nullptr, i->location());
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
    auto fname = _opToStr(i->op2());
    auto field = ast::checkedCast<type::Struct>(stype)->lookup(fname);
    assert(field);

    auto op1 = cg()->llvmValue(i->op1());
    auto op3 = cg()->llvmValue(i->op3(), field->type());
    cg()->llvmStructSet(stype, op1, fname, op3);
}

void StatementBuilder::visit(statement::instruction::struct_::Unset* i)
{
    auto stype = referencedType(i->op1());
    auto op1 = cg()->llvmValue(i->op1());
    cg()->llvmStructUnset(stype, op1, _opToStr(i->op2()));
}


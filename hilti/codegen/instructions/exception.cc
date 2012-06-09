
#include <hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::exception::New* i)
{
    auto type = ast::as<expression::Type>(i->op1())->typeValue();
    auto etype = ast::as<type::Exception>(type);

    CodeGen::value_list args;
    args.push_back(cg()->llvmExceptionTypeObject(etype));
    args.push_back(cg()->llvmConstNull());
    args.push_back(cg()->llvmLocationString(i->location()));
    auto result = cg()->llvmCallC("hlt_exception_new", args, false, false);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::exception::NewWithArg* i)
{
    auto type = ast::as<expression::Type>(i->op1())->typeValue();
    auto etype = ast::as<type::Exception>(type);

    auto arg = cg()->llvmValue(i->op2(), etype->argType());
    auto tmp = cg()->llvmAddTmp("excpt_arg", arg, true);
    tmp = builder()->CreateBitCast(tmp, cg()->llvmTypePtr());

    CodeGen::value_list args;
    args.push_back(cg()->llvmExceptionTypeObject(etype));
    args.push_back(tmp);
    args.push_back(cg()->llvmLocationString(i->location()));
    auto result = cg()->llvmCallC("hlt_exception_new", args, false, false);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::exception::Throw* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    cg()->llvmRaiseException(op1);
}


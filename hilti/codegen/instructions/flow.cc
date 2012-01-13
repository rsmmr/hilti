
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::flow::ReturnResult* i)
{
    auto func = current<declaration::Function>();
    auto op1 = cg()->llvmValue(i->op1(), as<type::Function>(func->function()->type())->result()->type());
    cg()->llvmReturn(op1);
}

void StatementBuilder::visit(statement::instruction::flow::ReturnVoid* i)
{
    cg()->llvmReturn();
}

void StatementBuilder::prepareCall(shared_ptr<Expression> func, shared_ptr<Expression> args, CodeGen::expr_list* call_params)
{
    auto expr = ast::as<expression::Constant>(args);

    if ( expr ) {
        // This is a short-cut to generate nicer code: if we have a constant
        // tuple, we use its element directly to avoid generating a struct first
        // just to then disassemble it again (LLVM should be able to optimize the
        // overhead in any case, but it's better readable this way.)
        for ( auto a : ast::as<constant::Tuple>(expr->constant())->value() )
            call_params->push_back(a);
    }

    else {
        // Standard case: dissect the tuple struct.
        auto ttype = ast::as<type::Tuple>(args->type());
        auto tval = cg()->llvmValue(args);

        int i = 0;
        for ( shared_ptr<hilti::Type> t : ttype->typeList() ) {
            llvm::Value* val = cg()->llvmExtractValue(tval, i++);
            auto expr = shared_ptr<Expression>(new expression::CodeGen(t, val));
            call_params->push_back(expr);
        }
    }
}

void StatementBuilder::visit(statement::instruction::flow::CallVoid* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op1(), i->op2(), &params);
    auto func = cg()->llvmValue(i->op1());
    auto ftype = ast::as<type::Function>(i->op1()->type());
    cg()->llvmCall(func, ftype, params);
}

void StatementBuilder::visit(statement::instruction::flow::CallResult* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op1(), i->op2(), &params);
    auto func = cg()->llvmValue(i->op1());
    auto ftype = ast::as<type::Function>(i->op1()->type());
    auto result = cg()->llvmCall(func, ftype, params);
    cg()->llvmStore(i->target(), result);
}

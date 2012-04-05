
#include <hilti.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::flow::ReturnResult* i)
{
    auto func = current<declaration::Function>();
    auto rtype = as<type::Function>(func->function()->type())->result()->type();
    auto op1 = cg()->llvmValue(i->op1(), rtype, false);
    cg()->llvmReturn(op1);
}

void StatementBuilder::visit(statement::instruction::flow::ReturnVoid* i)
{
    // Check if we are in a hook. If so, we return a boolean indicating that
    // hook execution has not been stopped.
    if ( ! current<declaration::Hook>() )
        cg()->llvmReturn();
    else
        cg()->llvmReturn(cg()->llvmConstInt(0, 1));
}

void StatementBuilder::prepareCall(shared_ptr<Expression> func, shared_ptr<Expression> args, CodeGen::expr_list* call_params)
{
    auto expr = ast::as<expression::Constant>(args);
    auto ftype = ast::as<type::Function>(func->type());
    auto params = ftype->parameters();
    auto p = params.begin();

    if ( expr && expr->isConstant() ) {
        // This is a short-cut to generate nicer code: if we have a constant
        // tuple, we use its element directly to avoid generating a struct first
        // just to then disassemble it again (LLVM should be able to optimize the
        // overhead in any case, but it's better readable this way.)
        for ( auto a : ast::as<constant::Tuple>(expr->constant())->value() ) {
            call_params->push_back(a);
            ++p;
        }
    }

    else {
        // Standard case: dissect the tuple struct.
        auto ttype = ast::as<type::Tuple>(args->type());
        auto tval = cg()->llvmValue(args, false);

        int i = 0;
        auto p = params.begin();

        for ( shared_ptr<hilti::Type> t : ttype->typeList() ) {
            llvm::Value* val = cg()->llvmExtractValue(tval, i++);
            auto expr = shared_ptr<Expression>(new expression::CodeGen(t, val));
            call_params->push_back(expr);
            ++p;
        }
    }

    // Add default values.
    while ( p != params.end() ) {
        auto def = (*p++)->defaultValue();
        assert(def);
        call_params->push_back(def);
    }
}

void StatementBuilder::visit(statement::instruction::flow::CallVoid* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op1(), i->op2(), &params);
    auto func = cg()->llvmValue(i->op1(), false);
    auto ftype = ast::as<type::Function>(i->op1()->type());
    cg()->llvmCall(func, ftype, params);
}

void StatementBuilder::visit(statement::instruction::flow::CallResult* i)
{
    CodeGen::expr_list params;
    prepareCall(i->op1(), i->op2(), &params);
    auto func = cg()->llvmValue(i->op1(), false);
    auto ftype = ast::as<type::Function>(i->op1()->type());
    auto result = cg()->llvmCall(func, ftype, params);
    cg()->llvmStore(i->target(), result);
}

void StatementBuilder::visit(statement::instruction::flow::IfElse* i)
{
    auto btype = shared_ptr<Type>(new type::Bool());
    auto op1 = cg()->llvmValue(i->op1(), btype);
    auto op2 = cg()->llvmValue(i->op2());
    auto op3 = cg()->llvmValue(i->op3());

    auto op2_bb = llvm::cast<llvm::BasicBlock>(op2);
    auto op3_bb = llvm::cast<llvm::BasicBlock>(op3);

    cg()->builder()->CreateCondBr(op1, op2_bb, op3_bb);
}

void StatementBuilder::visit(statement::instruction::flow::Jump* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op1_bb = llvm::cast<llvm::BasicBlock>(op1);

    cg()->builder()->CreateBr(op1_bb);
}


#include "../hilti.h"

#include "stmt-builder.h"
#include "codegen.h"
#include "util.h"

using namespace hilti;
using namespace codegen;

StatementBuilder::StatementBuilder(CodeGen* cg) : CGVisitor<>(cg, "StatementBuilder")
{
}

StatementBuilder::~StatementBuilder()
{
}

void StatementBuilder::llvmStatement(shared_ptr<Statement> stmt)
{
    call(stmt);
}

void StatementBuilder::visit(statement::Block* b)
{
    bool in_function = in<declaration::Function>();

    for ( auto d : b->Declarations() )
        call(d);

    for ( auto s : b->Statements() )
        call(s);
}

void StatementBuilder::visit(declaration::Variable* v)
{
    auto var = v->variable();

    if ( ! ast::isA<variable::Local>(var) )
        // GLobals are taken care of directly by the CodeGen.
        return;

    auto init = var->init() ? cg()->llvmValue(var->init()) : nullptr;

    cg()->llvmAddLocal(var->id()->name(), cg()->llvmType(var->type()), init);
}

void StatementBuilder::visit(declaration::Function* f)
{
    auto func = f->function();
    auto ftype = func->type();

    if ( ! func->body() )
        // No implementation, nothing to do here.
        return;

    assert(ftype->callingConvention() == type::function::HILTI);

    auto llvm_func = cg()->llvmFunction(func);

    cg()->pushFunction(llvm_func);
    call(func->body());
    cg()->llvmBuildExitBlock(func);
    cg()->popFunction();

    if ( f->exported() )
        cg()->llvmBuildCWrapper(func, llvm_func);
}

void StatementBuilder::visit(statement::instruction::integer::Add* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());
    auto result = builder()->CreateAdd(op1, op2);
    cg()->llvmStore(i->target(), result);
}

void StatementBuilder::visit(statement::instruction::integer::Sub* i)
{
    auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
    auto op2 = cg()->llvmValue(i->op2(), i->target()->type());
    auto result = builder()->CreateSub(op1, op2);
    cg()->llvmStore(i, result);
}

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

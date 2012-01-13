
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


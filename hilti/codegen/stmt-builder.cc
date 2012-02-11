
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

shared_ptr<Type> StatementBuilder::coerceTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const
{
    if ( op1->canCoerceTo(op2->type()) )
        return op2->type();

    if ( op2->canCoerceTo(op1->type()) )
        return op1->type();

    internalError("incompatible types in Instruction::coerceTypes()");
    return 0;
}

void StatementBuilder::visit(statement::Block* b)
{
    std::ostringstream comment;
    passes::Printer printer(comment);

    IRBuilder* builder = 0;

    if ( b->id() ) {
        builder = cg()->builderForLabel(b->id()->name());
        cg()->pushBuilder(builder);
    }

    for ( auto d : b->declarations() )
        call(d);

    for ( auto s : b->statements() ) {

        if ( cg()->debugLevel() > 0 && ! ast::isA<statement::Block>(s) ) {
            // Insert the source instruction as comment into the generated
            // code.
            comment.str(string());
            printer.run(s);
            passes::Printer p(comment);
            cg()->llvmInsertComment(comment.str());
        }

        call(s);
    }

    if ( builder )
        cg()->popBuilder();
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

    // Create shadow locals for non-const parameters so that we can modify
    // them.
    for ( auto p : ftype->parameters() ) {
        if ( p->constant() )
            continue;

        auto llvm_type = cg()->llvmType(p->type());
        auto llvm_init = cg()->llvmParameter(p);

        cg()->llvmAddLocal("__shadow_" + p->id()->name(), llvm_type, llvm_init);
    }

    call(func->body());

    cg()->llvmBuildExitBlock(func);

    cg()->popFunction();

    if ( f->exported() )
        cg()->llvmBuildCWrapper(func, llvm_func);
}


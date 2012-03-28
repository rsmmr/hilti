
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
    passes::Printer printer(comment, true);

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
            passes::Printer p(comment, true);
            cg()->llvmInsertComment(comment.str() + " (" + string(s->location()) + ")");

            // Send it also to the hilti-trace debug stream.
            cg()->llvmDebugPrint("hilti-trace", comment.str());
        }

        call(s);
        cg()->finishStatement();
    }

    if ( builder )
        cg()->popBuilder();
}

void StatementBuilder::visit(statement::Try* t)
{
    // Block for normal continuation.
    auto normal_cont = cg()->newBuilder("try-cont");

    for ( auto c : t->catches() ) {
        // Build code for catch block.
        auto builder = cg()->pushBuilder("catch-match");
        call(c);
        cg()->pushExceptionHandler(builder);

        // The Catch() will have left the builder on the stack that handles
        // fully catching the excepetion.  Add a branch and remove it.
        cg()->llvmClearException();
        cg()->llvmCreateBr(normal_cont);
        cg()->popBuilder();

        cg()->popBuilder();
    }

    call(t->block());
    cg()->llvmCreateBr(normal_cont);

    // Remove all our handlers.
    for ( auto c : t->catches() )
        cg()->popExceptionHandler();
}

void StatementBuilder::visit(statement::try_::Catch* c)
{
    // Create block for our code. Filled below.
    auto match = cg()->newBuilder("catch-block");

    // Create block for passing on to other handlers.
    auto no_match = cg()->pushBuilder();

    if ( cg()->topExceptionHandler() )
        cg()->llvmCreateBr(cg()->topExceptionHandler());

    else
        cg()->llvmRethrowException();

    cg()->popBuilder();

    // In the original block, check if it's our exception.

    auto rtype = ast::as<type::Reference>(c->type());
    assert(rtype);
    auto etype = ast::as<type::Exception>(rtype->argType());
    assert(etype);

    auto cexcpt = cg()->llvmCurrentException();

    CodeGen::value_list args;
    args.push_back(cexcpt);
    args.push_back(cg()->llvmExceptionTypeObject(etype));
    auto ours = cg()->llvmCallC("__hlt_exception_match", args, false, false);

    auto cond = builder()->CreateICmpNE(ours, cg()->llvmConstInt(0, 8));
    cg()->llvmCreateCondBr(cond, match, no_match);

    // Now build our code block.
    cg()->pushBuilder(match);

    if ( c->var() ) {
        // Initialize the local variable providing access to the exception.
        auto name = c->var()->internalName();
        auto addr = cg()->llvmAddLocal(name, c->type());

        // Can't init the local directly as they might end up in the wrong
        // block.
        cg()->llvmGCAssign(addr, cexcpt, c->type(), false);
    }

    call(c->block());

    // Leave on stack, Try's visit will remove.
}

void StatementBuilder::visit(declaration::Variable* v)
{
    // Ignore the Catch local here, we do that in its handlers.
    if ( in<statement::try_::Catch>() )
        return;

    auto var = v->variable();
    auto local = ast::as<variable::Local>(var);

    if ( ! local )
        // GLobals are taken care of directly by the CodeGen.
        return;

    auto init = local->init() ? cg()->llvmValue(local->init(), local->type(), true) : nullptr;
    auto name = local->internalName();
    assert(name.size());

    cg()->llvmAddLocal(name, local->type(), init);
}

void StatementBuilder::visit(declaration::Function* f)
{
    auto hook_decl = dynamic_cast<declaration::Hook*>(f);

    auto func = f->function();
    auto ftype = func->type();

    if ( ! func->body() )
        // No implementation, nothing to do here.
        return;

    assert(ftype->callingConvention() == type::function::HILTI || ftype->callingConvention() == type::function::HOOK);

    auto llvm_func = cg()->llvmFunction(func, true);

    cg()->pushFunction(llvm_func);

    // Create shadow locals for non-const parameters so that we can modify
    // them.
    for ( auto p : ftype->parameters() ) {
        if ( p->constant() )
            continue;

        auto init = cg()->llvmParameter(p);
        cg()->llvmCctor(init, p->type(), false);
        cg()->llvmAddLocal("__shadow_" + p->id()->name(), p->type(), init);
    }

    if ( hook_decl ) {
        // If a hook, first check whether its group is enabled.
        CodeGen::expr_list args { builder::integer::create(hook_decl->hook()->group()) };
        auto cont = cg()->llvmCall("hlt::hook_group_is_enabled", args, false);

        auto disabled = cg()->pushBuilder("disabled");
        cg()->llvmReturn(cg()->llvmConstInt(0, 1)); // Return false.
        cg()->popBuilder();

        auto enabled = cg()->newBuilder("enabled");
        cg()->llvmCreateCondBr(cont, enabled, disabled);

        cg()->pushBuilder(enabled); // Leave on stack.
    }

    call(func->body());

    cg()->llvmBuildExitBlock();

    cg()->popFunction();

    if ( f->exported() && func->type()->callingConvention() == type::function::HILTI )
        cg()->llvmBuildCWrapper(func, llvm_func);

    // If it's a hook, add meta information about the implementation.
    if ( hook_decl )
        cg()->llvmAddHookMetaData(hook_decl->hook(), llvm_func);
}


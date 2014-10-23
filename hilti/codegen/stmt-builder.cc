
#include "../hilti.h"

#include "stmt-builder.h"
#include "codegen.h"
#include "util.h"
#include "../options.h"

using namespace hilti;
using namespace codegen;

StatementBuilder::StatementBuilder(CodeGen* cg) : CGVisitor<>(cg, "StatementBuilder")
{
}

StatementBuilder::~StatementBuilder()
{
}

void StatementBuilder::llvmStatement(shared_ptr<Statement> stmt, bool cleanup)
{
    string tag;

    if ( cg()->options().profile >= 2 ) {
        auto instr = ast::tryCast<statement::instruction::Resolved>(stmt);

        // Skip terminators, their code generation may end the current basic
        // block so that we can't add our code for profiler.stop.
        if ( instr && ! instr->instruction()->terminator() ) {
            auto name = instr->instruction()->id()->name();
            if ( ! ::util::startsWith(name, "profiler.") &&
                 ! ::util::startsWith(name, "call") )
                // tag = ::util::fmt("instr/%s#%p", name, stmt.get());
                tag = ::util::fmt("instr/%s", name);
        }
    }

    if ( ! tag.empty() )
        cg()->llvmProfilerStart(tag);

    call(stmt);

    if ( ! tag.empty() )
        cg()->llvmProfilerStop(tag);
}

shared_ptr<Statement> StatementBuilder::currentStatement()
{
    return _stmts.size() ? _stmts.back() : nullptr;
}

passes::Liveness::LivenessSets StatementBuilder::liveness()
{
    return cg()->hiltiModule()->liveness()->liveness(currentStatement());
}

void StatementBuilder::preAccept(shared_ptr<ast::NodeBase> node)
{
    auto stmt = ast::tryCast<Statement>(node);
    auto block = ast::tryCast<statement::Block>(node);

    if ( ! stmt || block )
        return;

    _stmts.push_back(stmt);
}

void StatementBuilder::postAccept(shared_ptr<ast::NodeBase> node)
{
    cg()->llvmFlushLocalsClearedOnException();

    auto stmt = ast::tryCast<Statement>(node);
    auto block = ast::tryCast<statement::Block>(node);

    if ( ! stmt || block )
        return;

    // Note that individual instructions may (better: have to) run this
    // earlier already themselves, in which case this becomes a noop. That's
    // usually the case for terminators.
    cg()->llvmBuildInstructionCleanup();
    _stmts.pop_back();
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

        if ( ! cg()->block()->getTerminator() )
            cg()->llvmCreateBr(builder);

        cg()->pushBuilder(builder);
    }

    for ( auto d : b->declarations() )
        call(d);

    cg()->llvmBuildInstructionCleanup();

    for ( auto s : b->statements() ) {

        if ( cg()->options().debug && ! ast::isA<statement::Block>(s) && ! cg()->block()->getTerminator() ) {
            // Insert the source instruction as comment into the generated
            // code.
            comment.str(string());
            printer.run(s);
            passes::Printer p(comment, true);

            if ( comment.str().size() ) {
                cg()->llvmInsertComment(comment.str() + " (" + string(s->location()) + ")");

                string c = string(s->location());

                auto n = c.rfind("/");

                if ( n != string::npos )
                    c = c.substr(n + 1, string::npos);

                c +=  + ": " + comment.str();

                // Send it also to the hilti-trace debug stream.
                auto ins = ast::tryCast<statement::instruction::Resolved>(s);
                if ( ins && ! ins->instruction()->hideInDebugTrace() )
                    cg()->llvmDebugPrint("hilti-trace", c);
            }
        }

        llvmStatement(s);
    }
}

void StatementBuilder::visit(statement::Try* t)
{
#if 0
    // Block for try block.
    auto try_ = cg()->newBuilder("try");
    cg()->llvmCreateBr(try_);

    // Block for normal continuation.
    auto normal_cont = cg()->newBuilder("try-cont");

    cg()->pushEndOfBlockHandler(normal_cont);

    for ( auto c = t->catches().rbegin(); c != t->catches().rend(); c++ ) {
        // Build code for catch block but in reverse order.
        auto builder = cg()->pushBuilder("catch-match");

        call(*c);
        cg()->pushExceptionHandler(builder);

        // The Catch() will have left the builder on the stack that handles
        // fully catching the excepetion. Don't remove builder, there may
        // have been pushed more than ours in the meantime.
    }

    cg()->pushBuilder(try_);
    call(t->block());

    cg()->popEndOfBlockHandler();

    // Leave try_ on stack. After generating the block body it's potentially
    // not the top-most anymore.

    // Remove all our handlers.
    for ( auto c : t->catches() )
        cg()->popExceptionHandler();

    cg()->pushBuilder(normal_cont); // Leave on stack.
#endif
}

void StatementBuilder::visit(statement::try_::Catch* c)
{
#if 0
    // Create block for passing on to other handlers.
    IRBuilder* no_match = nullptr;

    if ( cg()->topExceptionHandler() ) {
        no_match = cg()->pushBuilder("next-handler");
        cg()->llvmCreateBr(cg()->topExceptionHandler());
        cg()->popBuilder();
    }

    else {
        no_match = cg()->pushBuilder("rethrow");
        cg()->llvmRethrowException();
        cg()->popBuilder();
    }

    // In the original block.

    // Create block for our code.
    IRBuilder* match = nullptr;

    if ( c->type() ) {
        // Check if it's our exception.
        match = cg()->newBuilder("caught-excpt");

        auto rtype = ast::as<type::Reference>(c->type());
        assert(rtype);
        auto etype = ast::as<type::Exception>(rtype->argType());
        assert(etype);

        auto cond = cg()->llvmMatchException(etype, cg()->llvmCurrentException());
        cg()->llvmCreateCondBr(cond, match, no_match);
    }

    else {
        // It's a catch all.
        match = cg()->newBuilder("catch-all");
        cg()->llvmCreateBr(match);
    }

    // Now build our code block.
    if ( match )
        cg()->pushBuilder(match);

    llvm::Value* addr = nullptr;

    if ( c->variable() ) {
        // Initialize the local variable providing access to the exception.
        auto name = c->variable()->internalName();
        auto local = cg()->llvmAddLocal(name, c->type());
        cg()->llvmCreateStore(cg()->llvmCurrentException(), local);
    }

    cg()->llvmClearException();

    call(c->block());

    // Leave on stack, Try's visit will remove.
#endif
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

    auto block = current<statement::Block>();
    assert(block);

    bool live = local->init() ? cg()->hiltiModule()->liveness()->liveIn(block, local) : false;
    auto init = live ? local->init() : nullptr;

    auto name = local->internalName();
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

    auto llvm_func = cg()->llvmFunction(func, (hook_decl != nullptr));

    cg()->pushFunction(llvm_func, true, false, false, ftype->callingConvention());

    cg()->setLeaveFunc(f);

    auto name = ::util::fmt("%s::%s", cg()->hiltiModule()->id()->name().c_str(), f->id()->name().c_str());

    if ( cg()->options().debug )
        cg()->llvmDebugPrint("hilti-flow", string("entering ") + name);

    if ( cg()->options().profile >= 1 )
        cg()->llvmProfilerStart(string("func/") + name);

    // Create shadow locals for non-const parameters so that we can modify
    // them.
    for ( auto p : ftype->parameters() ) {
        if ( p->constant() )
            continue;

        auto init = cg()->llvmParameter(p);
        auto shadow = "__shadow_" + p->id()->name();
        cg()->llvmAddLocal(shadow, p->type(), std::make_shared<expression::CodeGen>(p->type(), init));
    }

    if ( hook_decl ) {
        // If a hook, first check whether we have an exception raised from
        // the previous implementastion. It would be nicer if the linker did
        // this check in the merged hook function before it calls each
        // implementation, but it's tricky to get to the execution context
        // there.
        auto excpt = cg()->newBuilder("excpt-yes");
        auto cont = cg()->newBuilder("excpt-no");

        auto current = cg()->llvmCurrentException();
        auto is_null = cg()->llvmCreateIsNull(current);
        cg()->llvmCreateCondBr(is_null, cont, excpt);

        cg()->pushBuilder(excpt);
        cg()->llvmReturn(0, cg()->llvmConstInt(0, 1)); // Return false.
        cg()->popBuilder();

        cg()->pushBuilder(cont);

        // Check whether the hook's group is enabled.
        CodeGen::expr_list args { builder::integer::create(ftype->attributes().getAsInt(attribute::GROUP, 0)) };
        auto cont2 = cg()->llvmCall("hlt::hook_group_is_enabled", args, false, false);

        auto disabled = cg()->pushBuilder("disabled");
        cg()->llvmReturn(0, cg()->llvmConstInt(0, 1)); // Return false.
        cg()->popBuilder();

        auto enabled = cg()->newBuilder("enabled");
        cg()->llvmCreateCondBr(cont2, enabled, disabled);

        cg()->pushBuilder(enabled); // Leave on stack.
    }

    auto body = cg()->newBuilder("body");
    cg()->llvmCreateBr(body);

    cg()->pushBuilder(body);

    call(func->body());

    cg()->popFunction();

    if ( f->linkage() == Declaration::EXPORTED && func->type()->callingConvention() == type::function::HILTI )
        cg()->llvmBuildCWrapper(func);

    // If it's a hook, add meta information about the implementation.
    if ( hook_decl )
        cg()->llvmAddHookMetaData(hook_decl->hook(), llvm_func);

    // If it's an init function, add a call to our initialization code.
    if ( func->initFunction() ) {
        llvm::BasicBlock& block(cg()->llvmModuleInitFunction()->getEntryBlock());
        auto builder = util::newBuilder(cg()->llvmContext(), &block, false);
        std::vector<llvm::Value *> args = { cg()->llvmExecutionContext() };
        util::checkedCreateCall(builder, "call-init-func", llvm_func, args);
        delete builder;
    }
}

void StatementBuilder::visit(declaration::Type* t)
{
    if ( t->linkage() == Declaration::EXPORTED )
        // Trigger generating type information.
        cg()->llvmRtti(t->type());
}

void StatementBuilder::visit(statement::ForEach* f)
{
    shared_ptr<type::Reference> r = ast::as<type::Reference>(f->sequence()->type());
    shared_ptr<Type> t = r ? r->argType() : f->sequence()->type();
    auto iterable = ast::as<type::trait::Iterable>(t);
    assert(iterable);

    auto var = f->body()->scope()->lookupUnique(f->id());
    assert(var);

    // Add the local iteration variable..
    auto v = ast::as<expression::Variable>(var)->variable();
    auto local = ast::as<variable::Local>(v);
    cg()->llvmAddLocal(local->internalName(), local->type());

    auto end = cg()->makeLocal("end",  iterable->iterType());
    auto iter = cg()->makeLocal("iter", iterable->iterType());
    auto cmp = cg()->makeLocal("cmp", builder::boolean::type());

    cg()->llvmInstruction(iter, instruction::operator_::Begin, f->sequence());
    cg()->llvmInstruction(end, instruction::operator_::End, f->sequence());

    auto cond = cg()->newBuilder("loop-cond");
    auto body = cg()->newBuilder("loop-body");
    auto cont = cg()->newBuilder("loop-end");

    cg()->pushEndOfBlockHandler(nullptr);

    cg()->llvmCreateBr(cond);

    cg()->pushBuilder(cond);
    cg()->llvmInstruction(cmp, instruction::operator_::Equal, iter, end);
    cg()->llvmCreateCondBr(cg()->llvmValue(cmp), cont, body);
    cg()->popBuilder();

    cg()->pushBuilder(body);
    cg()->llvmInstruction(var, instruction::operator_::Deref, iter);
    call(f->body());
    cg()->llvmInstruction(iter, instruction::operator_::Incr, iter);
    cg()->llvmCreateBr(cond);
    cg()->popBuilder();

    cg()->popEndOfBlockHandler();

    cg()->pushBuilder(cont);

    // Leqe on stack.
}

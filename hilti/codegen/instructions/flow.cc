
#include <hilti-intern.h>

#include "../stmt-builder.h"
#include "autogen/instructions.h"

using namespace hilti;
using namespace codegen;


void StatementBuilder::visit(statement::instruction::flow::ReturnResult* i)
{
    auto func = current<declaration::Function>();
    auto rtype = as<type::Function>(func->function()->type())->result()->type();
    auto op1 = cg()->llvmValue(i->op1(), rtype);

    cg()->llvmReturn(func->function()->type()->result()->type(), op1);
}

static void _doVoidReturn(StatementBuilder* sbuilder)
{
    auto func = sbuilder->current<declaration::Function>();

    // Check if we are in a hook. If so, we return a boolean indicating that
    // hook execution has not been stopped.
    if ( ! sbuilder->current<declaration::Hook>() )
        sbuilder->cg()->llvmReturn();
    else
        sbuilder->cg()->llvmReturn(0, sbuilder->cg()->llvmConstInt(0, 1));
}

void StatementBuilder::visit(statement::instruction::flow::ReturnVoid* i)
{
    _doVoidReturn(this);
}

void StatementBuilder::visit(statement::instruction::flow::BlockEnd* i)
{
    auto handler = cg()->topEndOfBlockHandler();

    if ( ! handler.first )
        return;

    if ( handler.second ) {
        // If we have a block to jump to on block end, go there.
        if ( ! cg()->builder()->GetInsertBlock()->getTerminator() )
            cg()->llvmCreateBr(handler.second);
    }

    else {
        // TODO: We catch this error here; can we do that in the validator?
        auto func = current<declaration::Function>();

        if ( func && ! ast::isA<type::Void>(func->function()->type()->result()->type()) ) {
            fatalError(i, "function does not return a value");
            return;
        }

        // Otherwise turn into a void return.
        _doVoidReturn(this);
    }
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
            call_params->push_back(a->coerceTo((*p)->type()));
            ++p;
        }
    }

    else {
        // Standard case: dissect the tuple struct.
        auto ttype = ast::as<type::Tuple>(args->type());
        auto tval = cg()->llvmValue(args);

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
        auto def = (*p++)->default_();
        assert(def);
        call_params->push_back(def);
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

void StatementBuilder::visit(statement::instruction::flow::CallCallableVoid* i)
{
    auto ctype = ast::as<type::Callable>(referencedType(i->op1()));

    CodeGen::expr_list params;
    auto op1 = cg()->llvmValue(i->op1());

    cg()->llvmCallableRun(ctype, op1);
}

void StatementBuilder::visit(statement::instruction::flow::CallCallableResult* i)
{
    auto ctype = ast::as<type::Callable>(referencedType(i->op1()));

    CodeGen::expr_list params;
    auto op1 = cg()->llvmValue(i->op1());

    auto result = cg()->llvmCallableRun(ctype, op1);

    cg()->llvmStore(i->target(), result);
}

void StatementBuilder::visit(statement::instruction::flow::Yield* i)
{
    auto fiber = cg()->llvmCurrentFiber();
    cg()->llvmFiberYield(fiber);
}

void StatementBuilder::visit(statement::instruction::flow::YieldUntil* i)
{
    auto fiber = cg()->llvmCurrentFiber();
    cg()->llvmFiberYield(fiber, i->op1()->type(), cg()->llvmValue(i->op1()));
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

void StatementBuilder::visit(statement::instruction::flow::Switch* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto default_ = llvm::cast<llvm::BasicBlock>(cg()->llvmValue(i->op2()));
    auto a1 = ast::as<expression::Constant>(i->op3());
    auto a2 = ast::as<constant::Tuple>(a1->constant());

    std::list<std::pair<llvm::Value*, llvm::BasicBlock*>> alts;

    bool all_const = true;

    for ( auto c : a2->value() ) {
        auto c1 = ast::as<expression::Constant>(c);
        auto c2 = ast::as<constant::Tuple>(c1->constant());

        auto v = c2->value();
        auto j = v.begin();
        auto val = cg()->llvmValue((*j++)->coerceTo(i->op1()->type()));
        auto block = llvm::cast<llvm::BasicBlock>(cg()->llvmValue(*j++));

        if ( ! llvm::isa<llvm::Constant>(val) )
            all_const = false;

        alts.push_back(std::make_pair(val, block));
    }

    if ( all_const && ast::isA<type::Integer>(i->op1()->type()) ) {
        // We optimize for constant integers here, for which LLVM directly
        // provides a switch statement.
        auto switch_ = cg()->builder()->CreateSwitch(op1, default_);

        for ( auto a : alts ) {
            auto c = llvm::cast<llvm::ConstantInt>(a.first);
            assert(c);
            switch_->addCase(c, a.second);
        }
    }

    else {
        // In all other cases, we build an if-else chain using the type's
        // standard comparision operator.

        auto cmp = cg()->makeLocal("switch-cmp", builder::boolean::type());

        for ( auto a : alts ) {
            cg()->llvmInstruction(cmp, instruction::operator_::Equal, i->op1(), builder::codegen::create(i->op1()->type(), a.first));
            auto match = cg()->builder()->CreateICmpEQ(cg()->llvmConstInt(1, 1), cg()->llvmValue(cmp));
            auto next_block = cg()->newBuilder("switch-chain");
            cg()->builder()->CreateCondBr(match, a.second, next_block->GetInsertBlock());
            cg()->pushBuilder(next_block);
        }

        // Do the default.
        cg()->builder()->CreateBr(default_);

    }
}


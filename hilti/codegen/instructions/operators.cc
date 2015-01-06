
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"
#include "../type-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::operator_::Begin* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::End* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Incr* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::IncrBy* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Deref* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Equal* i)
{
    assert(false); // Not used.
}

void StatementBuilder::visit(statement::instruction::operator_::Unequal* i)
{
    auto eq = cg()->makeLocal("eq", builder::boolean::type());
    cg()->llvmInstruction(eq, instruction::operator_::Equal, i->op1(), i->op2());
    auto result = cg()->builder()->CreateSelect(cg()->llvmValue(eq), cg()->llvmConstInt(0, 1), cg()->llvmConstInt(1, 1));
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::operator_::Assign* i)
{
    if ( ! i->target()->type()->attributes().has(attribute::HOIST) ) {
        auto op1 = cg()->llvmValue(i->op1(), i->target()->type());
        cg()->llvmStore(i, op1);
    }

    else {
        // Hoist into stack.
        //
        // TODO: Not sure we should hard-code local variables here, but
        // whereelse is a good place to do this?
        auto texpr = ast::checkedCast<expression::Variable>(i->target());
        auto tvar  = ast::checkedCast<variable::Local>(texpr->variable());
        auto dst = cg()->llvmLocal(tvar->internalName());
        cg()->llvmValueInto(dst, i->op1(), i->target()->type());
    }
}

void StatementBuilder::visit(statement::instruction::operator_::Unpack* i)
{
    auto iters = cg()->llvmValue(i->op1());
    auto begin = cg()->llvmTupleElement(i->op1()->type(), iters, 0, false);
    auto end = cg()->llvmTupleElement(i->op1()->type(), iters, 1, false);
    auto fmt = cg()->llvmValue(i->op2());

    auto tl = ast::as<type::Tuple>(i->target()->type())->typeList();
    auto ty = tl.front();

    llvm::Value* arg = i->op3() ? cg()->llvmValue(i->op3()) : nullptr;
    shared_ptr<Type> arg_type = i->op3() ? i->op3()->type() : nullptr;

    auto unpack_result = cg()->llvmUnpack(ty, begin, end, fmt, arg, arg_type, i->location());

    auto result = cg()->llvmTuple({ unpack_result.first, unpack_result.second });
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::operator_::Pack* i)
{
    auto value = cg()->llvmValue(i->op1());
    auto type = i->op1()->type();
    auto fmt = cg()->llvmValue(i->op2());
    llvm::Value* arg = i->op3() ? cg()->llvmValue(i->op3()) : nullptr;
    shared_ptr<Type> arg_type = i->op3() ? i->op3()->type() : nullptr;

    auto result = cg()->llvmPack(value, type, fmt, arg, arg_type, i->location());
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::operator_::Clear* i)
{
    cg()->llvmStore(i->op1(), cg()->typeInfo(i->op1()->type())->init_val);
}

void StatementBuilder::visit(statement::instruction::operator_::Clone* i)
{
    auto t = i->op1()->type();
    auto lt = cg()->llvmType(t);
    auto dst = cg()->builder()->CreateAlloca(lt, 0, "clone");

    auto op = cg()->llvmValue(i->op1());
    auto src = cg()->builder()->CreateAlloca(lt, 0, "src");
    cg()->llvmCreateStore(op, src);

    auto ti = cg()->llvmRtti(t);

    auto src_casted = cg()->builder()->CreateBitCast(src, cg()->llvmTypePtr());
    auto dst_casted = cg()->builder()->CreateBitCast(dst, cg()->llvmTypePtr());

    CodeGen::value_list vals = { dst_casted, ti, src_casted };
    cg()->llvmCallC("hlt_clone_deep", vals, true, true);

    // Comes back refed.
    cg()->llvmDtor(dst, t, true, "operator.clone");

    auto result = cg()->builder()->CreateLoad(dst);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::operator_::Hash* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::hash_object", args);
    cg()->llvmStore(i, result);
}


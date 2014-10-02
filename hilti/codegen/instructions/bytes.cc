
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::bytes::New* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args;
        auto result = cg()->llvmCall("hlt::bytes_new", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target() };
        cg()->llvmCall("hlt::bytes_new_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::Append* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::bytes_append", args);
}

void StatementBuilder::visit(statement::instruction::bytes::Concat* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1(), i->op2() };
        auto result = cg()->llvmCall("hlt::bytes_concat", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1(), i->op2() };
        cg()->llvmCall("hlt::bytes_concat_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::Cmp* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_cmp", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto cmp = cg()->llvmCall("hlt::bytes_cmp", args);
    auto result = cg()->builder()->CreateICmpEQ(cmp, cg()->llvmConstInt(0, 8));

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Copy* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1() };
        auto result = cg()->llvmCall("hlt::bytes_copy", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1() };
        cg()->llvmCall("hlt::bytes_copy_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::Diff* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::iterator_bytes_diff", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Empty* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_empty", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Freeze* i)
{
    auto cone = shared_ptr<constant::Integer>(new constant::Integer(1, 64, i->location()));
    auto one  = shared_ptr<expression::Constant>(new expression::Constant(cone, i->location()));

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(one);
    cg()->llvmCall("hlt::bytes_freeze", args);
}

void StatementBuilder::visit(statement::instruction::bytes::IsFrozenBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_is_frozen", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::IsFrozenIterBytes* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::iterator_bytes_is_frozen", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Length* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());

    auto result = cg()->llvmCall("hlt::bytes_len", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Contains* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_contains_bytes", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Find* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_find_bytes", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::FindAtIter* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_find_bytes_at_iter", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Offset* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::bytes_offset", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Sub* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1(), i->op2() };
        auto result = cg()->llvmCall("hlt::bytes_sub", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1(), i->op2() };
        cg()->llvmCall("hlt::bytes_sub_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::Trim* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    cg()->llvmCall("hlt::bytes_trim", args);
}

void StatementBuilder::visit(statement::instruction::bytes::Unfreeze* i)
{
    auto czero = shared_ptr<constant::Integer>(new constant::Integer(0, 64, i->location()));
    auto zero  = shared_ptr<expression::Constant>(new expression::Constant(czero, i->location()));

    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(zero);
    cg()->llvmCall("hlt::bytes_freeze", args);
}

void StatementBuilder::visit(statement::instruction::bytes::ToIntFromAscii* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::bytes_to_int", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::ToIntFromBinary* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::bytes_to_int_binary", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Lower* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1() };
        auto result = cg()->llvmCall("hlt::bytes_lower", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1() };
        cg()->llvmCall("hlt::bytes_lower_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::Upper* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1() };
        auto result = cg()->llvmCall("hlt::bytes_upper", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1() };
        cg()->llvmCall("hlt::bytes_upper_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::StartsWith* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::bytes_starts_with", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Strip* i)
{
    auto tb = builder::reference::type(builder::bytes::type());
    auto op2 = i->op2() ? cg()->llvmValue(i->op2()) : cg()->llvmEnum("Hilti::Side::Both");
    auto op3 = i->op3() ? cg()->llvmValue(i->op3()) : cg()->llvmConstNull(cg()->llvmType((tb)));

    auto ltb = builder::codegen::create(tb, op3);
    auto side = builder::codegen::create(cg()->typeByName("Hilti::Side"), op2);

    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1(), side, ltb};
        auto result = cg()->llvmCall("hlt::bytes_strip", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1(), side, ltb };
        cg()->llvmCall("hlt::bytes_strip_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::Split1* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::bytes_split1", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Split* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::bytes_split", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Join* i)
{
    if ( ! i->hoisted() ) {
        CodeGen::expr_list args = { i->op1(), i->op2() };
        auto result = cg()->llvmCall("hlt::bytes_join", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->target(), i->op1(), i->op2() };
        cg()->llvmCall("hlt::bytes_join_hoisted", args);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::AppendObject* i)
{
    CodeGen::expr_list args = { i->op1(), i->op2() };
    cg()->llvmCall("hlt::bytes_append_object", args);
}

void StatementBuilder::visit(statement::instruction::bytes::RetrieveObject* i)
{
    auto t = i->target()->type();

    CodeGen::expr_list args = { i->op1(), builder::type::create(t) };
    auto voidp = cg()->llvmCall("hlt::bytes_retrieve_object", args);
    auto casted = cg()->builder()->CreateBitCast(voidp, cg()->llvmTypePtr(cg()->llvmType(t)));
    cg()->llvmStore(i, cg()->builder()->CreateLoad(casted));
}

void StatementBuilder::visit(statement::instruction::bytes::AtObject* i)
{
    if ( ! i->op2() ) {
        CodeGen::expr_list args = { i->op1() };
        auto result = cg()->llvmCall("hlt::bytes_at_object", args);
        cg()->llvmStore(i, result);
    }

    else {
        CodeGen::expr_list args = { i->op1(), i->op2() };
        auto result = cg()->llvmCall("hlt::bytes_at_object_of_type", args);
        cg()->llvmStore(i, result);
    }
}

void StatementBuilder::visit(statement::instruction::bytes::SkipObject* i)
{
    CodeGen::expr_list args = { i->op1() };
    auto result = cg()->llvmCall("hlt::bytes_skip_object", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::NextObject* i)
{
    CodeGen::expr_list args = { i->op1() };
    auto result = cg()->llvmCall("hlt::bytes_next_object", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::AppendMark* i)
{
    CodeGen::expr_list args = { i->op1() };
    cg()->llvmCall("hlt::bytes_append_mark", args);
}

void StatementBuilder::visit(statement::instruction::bytes::NextMark* i)
{
    CodeGen::expr_list args = { i->op1() };
    auto result = cg()->llvmCall("hlt::bytes_next_mark", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::bytes::Index* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());

    auto result = cg()->llvmCall("hlt::iterator_bytes_index", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Begin* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::bytes_begin", args);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::End* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::bytes_end", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Incr* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_bytes_incr", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::IncrBy* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_bytes_incr_by", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Equal* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    args.push_back(i->op2());
    auto result = cg()->llvmCall("hlt::iterator_bytes_eq", args);

    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::iterBytes::Deref* i)
{
    CodeGen::expr_list args;
    args.push_back(i->op1());
    auto result = cg()->llvmCall("hlt::iterator_bytes_deref", args);

    cg()->llvmStore(i, result);
}



#include "cg-operator-common.h"
#include <binpac/autogen/operators/map.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(ctor::Map* m)
{
    auto mtype = ast::checkedCast<type::Map>(m->type());
    auto ktype = cg()->hiltiType(mtype->keyType());
    auto vtype = cg()->hiltiType(mtype->valueType());

    hilti::builder::map::element_list elems;

    for ( auto e : m->elements() ) {
        auto k = cg()->hiltiExpression(e.first);
        auto v = cg()->hiltiExpression(e.second);
        elems.push_back(std::make_pair(k, v));
    }

    auto result = hilti::builder::map::create(ktype, vtype, elems, nullptr, ::hilti::AttributeSet(), m->location());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::map::In* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    auto result = cg()->builder()->addTmp("exists", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(result, hilti::instruction::map::Exists, op2, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::map::Index* i)
{
    auto mtype = ast::type::checkedTrait<type::Map>(i->op1()->type());
    auto ktype = cg()->hiltiType(mtype->keyType());
    auto vtype = cg()->hiltiType(mtype->valueType());

    auto result = builder()->addTmp("elem", vtype);
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::map::Get, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::map::IndexAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    auto op3 = cg()->hiltiExpression(i->op3());
    cg()->builder()->addInstruction(hilti::instruction::map::Insert, op1, op2, op3);
    setResult(op3);
}

void CodeBuilder::visit(expression::operator_::map::Delete* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(hilti::instruction::map::Remove, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::map::Size* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto result = cg()->builder()->addTmp("size", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::map::Size, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::map::Get* i)
{
    auto mtype = ast::type::checkedTrait<type::Map>(i->op1()->type());
    auto ktype = cg()->hiltiType(mtype->keyType());
    auto vtype = cg()->hiltiType(mtype->valueType());

    auto result = builder()->addTmp("elem", vtype);
    auto op1 = cg()->hiltiExpression(i->op1());
    auto idx = cg()->hiltiExpression(callParameter(i->op3(), 0));

    auto def = callParameter(i->op3(), 1);

    if ( def ) {
        auto hdef = cg()->hiltiExpression(def);
        cg()->builder()->addInstruction(result, hilti::instruction::map::GetDefault, op1, idx, hdef);
    }

    else
        cg()->builder()->addInstruction(result, hilti::instruction::map::Get, op1, idx);

    setResult(result);
}


void CodeBuilder::visit(expression::operator_::map::Clear* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(nullptr, hilti::instruction::map::Clear, op1);
    setResult(op1);
}

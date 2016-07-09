
#include "cg-operator-common.h"
#include <binpac/autogen/operators/address.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::Address* a)
{
    auto c = hilti::builder::address::create(a->value());
    setResult(c);
}

void CodeBuilder::visit(expression::operator_::address::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(binpac::expression::operator_::address::Family* i)
{
    auto addr = cg()->hiltiExpression(i->op1());
    auto family =
        cg()->builder()->addTmp("family", hilti::builder::type::byName("Hilti::AddrFamily"));
    cg()->builder()->addInstruction(family, hilti::instruction::address::Family, addr);
    setResult(family);
}

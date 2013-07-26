
#include <hilti/hilti-intern.h>

#include "../stmt-builder.h"
#include "libhilti/port.h"

using namespace hilti;
using namespace codegen;

void StatementBuilder::visit(statement::instruction::port::Equal* i)
{
    auto op1 = cg()->llvmValue(i->op1());
    auto op2 = cg()->llvmValue(i->op2());

    auto v1 = cg()->llvmExtractValue(op1, 0);
    auto v2 = cg()->llvmExtractValue(op2, 0);
    auto cmp1 = cg()->builder()->CreateICmpEQ(v1, v2);

    v1 = cg()->llvmExtractValue(op1, 1);
    v2 = cg()->llvmExtractValue(op2, 1);
    auto cmp2 = cg()->builder()->CreateICmpEQ(v1, v2);

    auto result = cg()->builder()->CreateAnd(cmp1, cmp2);
    cg()->llvmStore(i, result);
}

void StatementBuilder::visit(statement::instruction::port::Protocol* i)
{
    auto op1 = cg()->llvmValue(i->op1());

    auto v = cg()->llvmExtractValue(op1, 1);
    auto cmp = cg()->builder()->CreateICmpEQ(v, cg()->llvmConstInt(HLT_PORT_TCP, 8));

    auto tcp = cg()->llvmEnum("Hilti::Protocol::TCP");
    auto udp = cg()->llvmEnum("Hilti::Protocol::UDP");

    auto result = cg()->builder()->CreateSelect(cmp, tcp, udp);
    cg()->llvmStore(i, result);
}

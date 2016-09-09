
#include "cg-operator-common.h"
#include <binpac/autogen/operators/bitfield.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(expression::operator_::bitfield::Attribute* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto btype = ast::rtti::checkedCast<type::Bitfield>(i->op1()->type());
    auto attr = ast::rtti::checkedCast<expression::MemberAttribute>(i->op2());

    int idx = 0;

    for ( auto b : btype->bits() ) {
        if ( attr->id()->name() != b->id()->name() ) {
            ++idx;
            continue;
        }

        auto t = cg()->hiltiType(b->fieldType());
        auto result = builder()->addTmp("bits", t);
        cg()->builder()->addInstruction(result, hilti::instruction::tuple::Index, op1,
                                        ::hilti::builder::integer::create(idx));

        setResult(result);
        break;
    }
}

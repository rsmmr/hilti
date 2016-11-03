
#include "cg-operator-common.h"
// #include "autogen/operators/port.h"

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(constant::Port* p)
{
    auto c = hilti::builder::port::create(p->value());
    setResult(c);
}

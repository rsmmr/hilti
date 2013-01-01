
#include "cg-operator-common.h"
// #include "autogen/operators/port.h"

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::Port* p)
{
    auto c = hilti::builder::port::create(p->value());
    setResult(c);
}


#include "cg-operator-common.h"
// #include "autogen/operators/string.h"

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(constant::String* s)
{
    auto result = hilti::builder::string::create(s->value(), s->location());
    setResult(result);
}


#include "cg-operator-common.h"
// #include "autogen/operators/string.h"

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(constant::String* s)
{
    auto result = hilti::builder::string::create(s->value(), s->location());
    setResult(result);
}

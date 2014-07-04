
#include "cg-operator-common.h"
// #include "autogen/operators/regexp.h"

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(ctor::RegExp* r)
{
    hilti::builder::regexp::re_pattern_list patterns;

    for ( auto p : r->patterns() )
        patterns.push_back(p);

    auto result = hilti::builder::regexp::create(patterns, hilti::AttributeSet(), r->location());
    setResult(result);
}


#include "cg-operator-common.h"
// #include "autogen/operators/regexp.h"

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(ctor::RegExp* r)
{
    hilti::builder::regexp::re_pattern_list patterns;

    for ( auto p : r->patterns() )
        patterns.push_back(std::make_tuple(p.first, p.second));

    auto result = hilti::builder::regexp::create(patterns, r->location());
    setResult(result);
}

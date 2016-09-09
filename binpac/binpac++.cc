
#include "binpac++.h"

// Automatically generated.
namespace binpac {
extern void __registerAllOperators();
}

void binpac::init()
{
    ast::rtti::RTTI::init();
    binpac::init_pac2();
}

void binpac::init_pac2()
{
    binpac::__registerAllOperators();
}

string binpac::version()
{
    return configuration().version;
}

binpac::operator_list binpac::operators()
{
    operator_list ops;

    for ( auto i : OperatorRegistry::globalRegistry()->getAll() )
        ops.push_back(i.second);

    return ops;
}


#include "binpac.h"

// Automatically generated.
namespace binpac { extern void __registerAllOperators(); }

void binpac::init()
{
    binpac::__registerAllOperators();
}

string binpac::version()
{
    return CONFIG_BINPAC_VERSION;
}

binpac::operator_list binpac::operators()
{
    operator_list ops;

    for ( auto i : OperatorRegistry::globalRegistry()->getAll() )
        ops.push_back(i.second);

    return ops;
}

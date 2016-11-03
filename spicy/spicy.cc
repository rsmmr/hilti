
#include "spicy.h"

// Automatically generated.
namespace spicy {
extern void __registerAllOperators();
}

void spicy::init()
{
    ast::rtti::RTTI::init();
    spicy::init_spicy();
}

void spicy::init_spicy()
{
    spicy::__registerAllOperators();
}

string spicy::version()
{
    return configuration().version;
}

spicy::operator_list spicy::operators()
{
    operator_list ops;

    for ( auto i : OperatorRegistry::globalRegistry()->getAll() )
        ops.push_back(i.second);

    return ops;
}

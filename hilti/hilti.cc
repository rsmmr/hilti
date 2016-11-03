
#include "hilti-intern.h"
#include "hilti/autogen/hilti-config.h"
#include "instructions/optypes.h"

// Auto-generated in ${autogen}/instructions-register.cc
namespace hilti {
extern void __registerAllInstructions();
}

void hilti::init()
{
    ast::rtti::RTTI::init();
    hilti::optype::__init();
    hilti::__registerAllInstructions();
}

extern string hilti::version()
{
    return configuration().version;
}

instruction_list hilti::instructions()
{
    instruction_list instrs;

    for ( auto i : InstructionRegistry::globalRegistry()->getAll() )
        instrs.push_back(i.second);

    return instrs;
}

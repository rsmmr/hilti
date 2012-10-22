
#include "hilti-intern.h"
#include "autogen/hilti-config.h"

// Auto-generated in ${autogen}/instructions-register.cc
extern void __registerAllInstructions();

void hilti::init()
{
    __registerAllInstructions();
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

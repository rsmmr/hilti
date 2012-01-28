
#include "../hilti.h"

#include "all.h"

using namespace hilti;

void InstructionRegistry::addAll()
{
    // All instructions must be registered here.
    implementInstruction(string::Cat);
    implementInstruction(integer::Add);
    implementInstruction(integer::Sub);
    implementInstruction(flow::ReturnResult);
    implementInstruction(flow::ReturnVoid);
    implementInstruction(flow::CallVoid);
    implementInstruction(flow::CallResult);
}

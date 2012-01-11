
#include "hilti.h"

using namespace hilti::passes;

InstructionResolver::~InstructionResolver()
{
}

void InstructionResolver::visit(statement::Instruction* s)
{
    auto matches = theInstructionRegistry->getMatching(s->id(), s->operands());

    auto instr = s->sharedPtr<statement::Instruction>();

    switch ( matches.size() ) {
     case 0:
        if ( ! theInstructionRegistry->has(s->id()->name()) )
            error(s, util::fmt("unknown instruction %s", s->id()->name().c_str()));
        else
            error(s, util::fmt("operands do not match for instruction %s", s->id()->name().c_str()));
        break;

     case 1: {
        // Everthing is fine. Replace with the actual instruction.
        auto new_stmt = theInstructionRegistry->resolveStatement(*matches.begin(), instr);
        instr->parent()->replaceChild(instr, new_stmt);
        break;
     }

     default:
        error(s, util::fmt("use of overloaded instruction %s is ambigious", s->id()->name().c_str()));
    }
}

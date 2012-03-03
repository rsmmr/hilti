
#include "hilti.h"
#include "builder.h"

using namespace hilti::passes;

InstructionResolver::~InstructionResolver()
{
}

void InstructionResolver::visit(statement::Instruction* s)
{
    auto matches = theInstructionRegistry->getMatching(s->id(), s->operands());
    auto instr = s->sharedPtr<statement::Instruction>();

    string error_msg;

    switch ( matches.size() ) {
     case 0:
         if ( ! theInstructionRegistry->has(s->id()->name()) ) {

             auto body = current<statement::Block>();

             if ( body->scope()->lookup(s->id()) ) {
                 hilti::instruction::Operands ops;
                 /// ops.push_back(s->operands().front()); // Target. XXX Doesn't work, infinite loop in ast it seems.
                 ops.push_back(builder::id::create(s->id(), s->location())); // Op1.
                 auto i = std::make_shared<statement::Instruction>(std::make_shared<ID>("assign"), ops, s->location());
                 instr->parent()->replaceChild(instr, i);
                 return;
             }

             error(s, util::fmt("unknown instruction %s", s->id()->name().c_str()));
             return;
         }

        else
            error_msg = util::fmt("operands do not match instruction %s\n", s->id()->name().c_str());

        break;

     case 1: {
         // Everthing is fine. Replace with the actual instruction.
         auto new_stmt = theInstructionRegistry->resolveStatement(*matches.begin(), instr);
         instr->parent()->replaceChild(instr, new_stmt);
         return;
     }

     default:
        error_msg = util::fmt("use of overloaded instruction %s is ambigious", s->id()->name().c_str());
    }

    error_msg += "  got:\n";
    error_msg += "    " + s->signature() + "\n";

    error_msg += "  expected one of:\n";

    for ( auto a : theInstructionRegistry->byName(s->id()->name()) )
        error_msg += "    " + string(*a) + "\n";

    error(s, error_msg);
}

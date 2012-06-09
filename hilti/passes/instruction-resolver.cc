
#include "hilti-intern.h"

using namespace hilti::passes;

InstructionResolver::~InstructionResolver()
{
}

void InstructionResolver::visit(statement::instruction::Unresolved* s)
{
    auto instr = s->sharedPtr<statement::instruction::Unresolved>();

    if ( instr->instruction() ) {
        // We already know the instruction, just need to transfer the operands over.
        auto new_stmt = InstructionRegistry::globalRegistry()->resolveStatement(instr->instruction(), instr);
        instr->parent()->replaceChild(instr, new_stmt);
        return;
    }

    auto id = s->id();
    auto name = s->id()->name();

    if ( util::startsWith(name, ".op.") )
        // These are dummy instructions used only to provide a single class
        // for the builder interface to access overloaded operators. We use
        // the non-prefixed name instead to do the lookup.
        id = std::make_shared<ID>(name.substr(4, std::string::npos));

    auto matches = InstructionRegistry::globalRegistry()->getMatching(id, s->operands());

    string error_msg;

    switch ( matches.size() ) {
     case 0:
         if ( ! InstructionRegistry::globalRegistry()->has(id->name()) ) {
             // Not a known instruction. See if this is a legal ID. If so,
             // replace with an assign expression.

             auto body = current<statement::Block>();

             if ( body->scope()->lookup(s->id()) ) {
                 auto assign = std::make_shared<ID>("assign");
                 instruction::Operands ops = { instr->target(), builder::id::create(s->id()), nullptr, nullptr };
                 auto matches = InstructionRegistry::globalRegistry()->getMatching(assign, ops);

                 if ( matches.size() != 1 ) {
                     error(s, util::fmt("unsupporte assign operation (%d)", matches.size()));
                     return;
                 }

                 auto as = (*matches.front()->factory())(matches.front(), ops, instr->location());
                 instr->parent()->replaceChild(instr, as);
                 return;
             }

              error(s, util::fmt("unknown instruction %s", s->id()->name().c_str()));
             return;
         }

        else
            error_msg = util::fmt("operands do not match instruction %s\n", id->name().c_str());

        break;

     case 1: {
         // Everthing is fine. Replace with the actual instruction.
         auto new_stmt = InstructionRegistry::globalRegistry()->resolveStatement(*matches.begin(), instr);
         instr->parent()->replaceChild(instr, new_stmt);
         return;
     }

     default:
        error_msg = util::fmt("use of overloaded instruction %s is ambigious", id->name().c_str());
    }

    error_msg += "  got:\n";
    error_msg += "    " + s->signature() + "\n";

    error_msg += "  expected one of:\n";

    for ( auto a : InstructionRegistry::globalRegistry()->byName(id->name()) )
        error_msg += "    " + string(*a) + "\n";

    error(s, error_msg);
}

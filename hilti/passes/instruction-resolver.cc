
#include "hilti/hilti-intern.h"

using namespace hilti::passes;

InstructionResolver::~InstructionResolver()
{
}

void InstructionResolver::processInstruction(shared_ptr<statement::Instruction> instr, shared_ptr<ID> id)
{
#if 0
    std::cerr << "<<" << s->render() << std::endl;

    int i = 0;
    for ( auto o : s->operands() ) {
        if ( o ) {
            std::cerr << "   " << i++ << " " << o->type()->render() << std::endl;
            o->type()->dump(std::cerr);
        }
    }

    std::cerr << "<<" << s->render() << std::endl;
#endif

    auto matches = InstructionRegistry::globalRegistry()->getMatching(id, instr->operands());

    string error_msg;

    switch ( matches.size() ) {
     case 0:
         if ( ! InstructionRegistry::globalRegistry()->has(id->name()) ) {
             // Not a known instruction. See if this is a legal ID. If so,
             // replace with an assign expression.

             auto body = current<statement::Block>();

             if ( auto expr = body->scope()->lookupUnique(id) ) {
                 auto assign = std::make_shared<ID>("assign");
                 instruction::Operands ops = { instr->target(), expr, nullptr, nullptr };
                 auto matches = InstructionRegistry::globalRegistry()->getMatching(assign, ops);

                 if ( matches.size() != 1 ) {
                     error(instr, util::fmt("unsupporte assign operation (%d)", matches.size()));
                     return;
                 }

                 auto as = (*matches.front()->factory())(matches.front(), ops, instr->location());
                 instr->replace(as);
                 return;
             }

             if ( _report_errors )
                 error(instr, util::fmt("unknown instruction %s", id->name().c_str()));

             return;
         }

        else
            error_msg = util::fmt("operands do not match instruction %s\n", id->name().c_str());

        break;

     case 1: {
         // Everthing is fine. Replace with the actual instruction.
         auto new_stmt = InstructionRegistry::globalRegistry()->resolveStatement(*matches.begin(), instr);
         instr->replace(new_stmt);
         return;
     }

     default:
        error_msg = util::fmt("use of overloaded instruction %s is ambigious", id->name().c_str());
    }

    error_msg += "  got:\n";
    error_msg += "    " + instr->signature() + "\n";

    error_msg += "  expected one of:\n";

    for ( auto a : InstructionRegistry::globalRegistry()->byName(id->name()) )
        error_msg += "    " + string(*a) + "\n";

    if ( _report_errors )
        error(instr, error_msg);
}

void InstructionResolver::visit(statement::instruction::Unresolved* s)
{
    auto instr = s->sharedPtr<statement::instruction::Unresolved>();

    if ( instr->instruction() && ! util::startsWith(instr->instruction()->id()->name(), ".op.") ) {
        // We already know the instruction, just need to transfer the operands over.
        auto new_stmt = InstructionRegistry::globalRegistry()->resolveStatement(instr->instruction(), instr);

        instr->replace(new_stmt);

        return;
    }

    auto id = s->id();
    auto name = s->id()->name();

    if ( util::startsWith(name, ".op.") )
        // These are dummy instructions used only to provide a single class
        // for the builder interface to access overloaded operators. We use
        // the non-prefixed name instead to do the lookup.
        id = std::make_shared<ID>(name.substr(4, std::string::npos));

    return processInstruction(instr, id);
}

void InstructionResolver::visit(statement::instruction::Resolved* s)
{
    auto instr = s->sharedPtr<statement::instruction::Resolved>();

    auto id = s->id();
    auto name = s->id()->name();

    if ( ! util::startsWith(name, ".op.") )
        return;

    // These are dummy instructions used only to provide a single class
    // for the builder interface to access overloaded operators. We use
    // the non-prefixed name instead to do the lookup.
    id = std::make_shared<ID>(name.substr(4, std::string::npos));
    return processInstruction(instr, id);
}

void InstructionResolver::visit(statement::Block* block)
{
    // Link the statements within the block. Due to preorder traversal, this
    // will run after intstruction have been resolved.

    shared_ptr<Statement> prev = nullptr;

    for ( auto s : block->statements() ) {
        if ( prev )
            prev->setSuccessor(s);

        prev = s;
    }

    if ( prev )
        prev->setSuccessor(nullptr);
}

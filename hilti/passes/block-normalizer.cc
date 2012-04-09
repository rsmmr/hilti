
#include "../statement.h"
#include "../module.h"

#include "block-normalizer.h"
#include "printer.h"

using namespace hilti;
using namespace passes;

void BlockNormalizer::visit(statement::Block* b)
{
    if ( b->terminated() )
        return;

    auto p = current<statement::Block>();

    if ( ! p )
        return;

    if ( ! (p->parent() && ast::isA<Function>(p->parent()->sharedPtr<Node>())) )
        // Don't normalize nested blocks, the code generator needs to take
        // care of those.
        return;

    // Find ourselves in parent's list.
    auto stmts = p->statements();
    auto self = stmts.begin();

    for ( ; self != stmts.end(); self++ ) {
        if ( self->get() == b )
            break;
    }

    if ( self == stmts.end() )
        return;

    auto succ = self;
    ++succ;

    shared_ptr<statement::instruction::Unresolved> terminator = 0;

    if ( succ != stmts.end() ) {
        // If there's a succesor block, add a branch.
        //
        // FIXME: Do we want/need to generalize to when succ statement is not
        // a block?
        auto next = ast::as<statement::Block>(*succ);
        assert(next);

        // FIXME: Anonymus blocks not supported here currently.
        auto label = next->id();
        assert(label);

        hilti::instruction::Operands ops;

        auto c = shared_ptr<hilti::constant::Label>(new hilti::constant::Label(label->name()));
        auto e = shared_ptr<hilti::expression::Constant>(new hilti::expression::Constant(c));
        ops.push_back(shared_ptr<Expression>()); // No target.
        ops.push_back(e);

        terminator = shared_ptr<hilti::statement::instruction::Unresolved>(
            new hilti::statement::instruction::Unresolved("jump", ops, b->location()));
        terminator->setInternal();
    }

    else {
        // Otherwise a return.void statement.
        hilti::instruction::Operands no_ops;

        terminator = shared_ptr<hilti::statement::instruction::Unresolved>(
            new hilti::statement::instruction::Unresolved("return.void", no_ops, b->location()));
        terminator->setInternal();
    }

    assert(terminator);
    b->addStatement(terminator);
}



#include "../statement.h"
#include "../module.h"

#include "block-normalizer.h"
#include "printer.h"

using namespace hilti;
using namespace passes;

void BlockNormalizer::visit(statement::Block* b)
{
    b->removeUseless();

    if ( b->terminated() )
        return;

    // Determine where to continue execution after block.
    //
    // 1. If we have a block sibling, go there.
    // 2. Otherwise, add a block_end instruction.

    shared_ptr<statement::instruction::Unresolved> terminator = 0;
    statement::Block* next = nullptr;

    auto parent_ = parent<statement::Block>();

    if ( parent_ ) {
        ast::NodeBase* n = b;

        while ( n ) {
            n = parent_->siblingOfChild(n);
            next = dynamic_cast<statement::Block*>(n);

            if ( next )
                break;
        }
    }

    // FIXME: Do we want/need to generalize to when succ statement is not
    // a block?
    if ( next ) {
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
        b->addStatement(terminator);

        return;
    }

    // Otherwise an internal block.end statement.
    hilti::instruction::Operands no_ops;

    terminator = shared_ptr<hilti::statement::instruction::Unresolved>(
                 new hilti::statement::instruction::Unresolved("block.end", no_ops, b->location()));

    terminator->setInternal();
    b->addStatement(terminator);
}


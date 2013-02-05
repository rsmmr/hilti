
#include "../statement.h"
#include "../module.h"

#include "block-normalizer.h"
#include "printer.h"
#include "builder/nodes.h"
#include "autogen/instructions.h"

using namespace hilti;
using namespace passes;

void BlockNormalizer::visit(declaration::Function* f)
{
    _anon_cnt = 0;
}

void BlockNormalizer::visit(statement::Block* b)
{
    // - Merge anonymous blocks with their unterminated predecessors
    // - Adds jumps from unterminated blocks to their successors.
    auto ostmts = b->statements();
    auto nstmts = statement::Block::stmt_list();

    auto pred = ostmts.end();

    std::set<shared_ptr<statement::Block>> now_terminated;

    for ( auto cur = ostmts.begin(); cur != ostmts.end(); cur++ ) {

        // Make sure the AST tree relationships reflect previous
        // normalizations.
        (*cur)->removeFromParents();
        b->addChild(*cur);

        if ( pred == ostmts.end() ) {
            nstmts.push_back(*cur);
            pred = cur;
            continue;
        }

        auto pred_block = ast::tryCast<statement::Block>(*pred);
        auto cur_block = ast::tryCast<statement::Block>(*cur);

        if ( ! (pred_block && cur_block) ) {
            nstmts.push_back(*cur);
            continue;
        }

        cur_block->scope()->setParent(b->scope());

        if ( cur_block->id() && ! pred_block->terminated() ) {
            hilti::instruction::Operands ops = { nullptr, builder::label::create(cur_block->id()->name()) };
            auto jump = std::make_shared<statement::instruction::Unresolved>(instruction::flow::Jump, ops);
            pred_block->addStatement(jump);
            now_terminated.insert(pred_block);
        }

        if ( cur_block->id() || pred_block->terminated() ) {
            nstmts.push_back(*cur);
            pred = cur;
            continue;
        }

        // Merge.

        for ( auto s : cur_block->statements() ) {
            s->removeFromParents();
            pred_block->addStatement(s);
        }

        for ( auto d : cur_block->declarations() ) {
            d->removeFromParents();
            pred_block->addDeclaration(d);
        }

        pred_block->scope()->mergeFrom(cur_block->scope());

    }

    b->setStatements(nstmts);

    // Add a block.end at the end of still unterminated blocks.
    for ( auto s : b->statements() ) {
        auto block = ast::tryCast<statement::Block>(s);

        if ( block && ! block->terminated() && now_terminated.find(block) == now_terminated.end() ) {
            hilti::instruction::Operands no_ops;
            auto end = std::make_shared<statement::instruction::Unresolved>(instruction::flow::BlockEnd, no_ops);
            block->addStatement(end);
        }
    }

    b->removeUseless();

#if 0

    if ( ! b->terminated() )
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

    if ( next ) {
        if ( ! next->id() )
            next->setID(builder::id::node(::util::fmt("__bn_anon_%d", ++_anon_cnt)));

        auto label = next->id();

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
#endif
}


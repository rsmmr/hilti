
#include "../builder/nodes.h"
#include "../module.h"
#include "../statement.h"

#include "block-normalizer.h"
#include "hilti/autogen/instructions.h"
#include "printer.h"

using namespace hilti;
using namespace passes;

BlockNormalizer::BlockNormalizer(bool instructions_normalized)
    : Pass<>("hilti::codegen::BlockNormalizer", true)
{
    _instructions_normalized = instructions_normalized;
}

bool BlockNormalizer::run(shared_ptr<hilti::Node> module)
{
    return processAllPreOrder(module);
}


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
        if ( pred == ostmts.end() ) {
            nstmts.push_back(*cur);
            pred = cur;
            continue;
        }

        auto pred_block = ast::rtti::tryCast<statement::Block>(*pred);
        auto cur_block = ast::rtti::tryCast<statement::Block>(*cur);

        if ( ! (pred_block && cur_block) ) {
            nstmts.push_back(*cur);
            continue;
        }

        cur_block->scope()->setParent(b->scope());

        if ( cur_block->id() && ! pred_block->terminated() ) {
            hilti::instruction::Operands ops = {nullptr,
                                                builder::label::create(cur_block->id()->name())};
            auto jump =
                std::make_shared<statement::instruction::Unresolved>(instruction::flow::Jump, ops);
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

    if ( _instructions_normalized ) {
        // Add a block.end at the end of still unterminated blocks.
        for ( auto s : b->statements() ) {
            auto block = ast::rtti::tryCast<statement::Block>(s);

            if ( block && ! block->terminated() &&
                 now_terminated.find(block) == now_terminated.end() ) {
                hilti::instruction::Operands no_ops;
                auto end = std::make_shared<
                    statement::instruction::Unresolved>(instruction::flow::BlockEnd, no_ops);
                block->addStatement(end);
            }

            b->removeUseless();
        }
    }
}


#include "../hilti-intern.h"
#include "../module.h"
#include "../scope.h"
#include "hilti/autogen/instructions.h"
#include "id-replacer.h"

#include "instruction-normalizer.h"

using namespace hilti;
using namespace passes;

InstructionNormalizer::InstructionNormalizer() : Pass<>("hilti::InstructionNormalizer")
{
}

bool InstructionNormalizer::run(shared_ptr<hilti::Node> module)
{
    return processAllPostOrder(module);
}

shared_ptr<Expression> InstructionNormalizer::_addLocal(shared_ptr<statement::Block> block, const string& hint, shared_ptr<Type> type, bool force_name, const Location& l)
{
    auto cnt = 0;
    string name;

    if ( force_name )
        name = hint;

    else {
        do {
            name = util::fmt("__%s_%d", hint, ++cnt);
        } while ( _names.find(name) != _names.end() );
    }

    _names.insert(name);

    auto id = std::make_shared<ID>(name, l);
    auto var = std::make_shared<variable::Local>(id, type, nullptr, l);
    auto decl = std::make_shared<declaration::Variable>(id, var, l);
    block->addDeclaration(decl);

    block->scope()->insert(id, std::make_shared<expression::Variable>(var));

    return std::make_shared<expression::Variable>(var, l);
}

std::pair<shared_ptr<statement::Block>, shared_ptr<Expression>> InstructionNormalizer::_addBlock(shared_ptr<statement::Block> parent, const string& hint, bool dont_add_yet, const Location& l)
{
    auto cnt = 0;

    string name;

    do {
        name = util::fmt("@__%s_%d", hint, ++cnt);
    } while ( _names.find(name) != _names.end() );

    _names.insert(name);

    auto id = std::make_shared<ID>(name, l);

    auto child = std::make_shared<statement::Block>(nullptr, id, l);
    auto expr = builder::label::create(name, l);

    parent->scope()->insert(id, std::make_shared<expression::Block>(child));

    if ( ! dont_add_yet )
        parent->addStatement(child);

    child->scope()->setParent(parent->scope());

    return std::make_pair(child, expr);
}

void InstructionNormalizer::_addInstruction(shared_ptr<statement::Block> block,
                            shared_ptr<Expression> target,
                            shared_ptr<Instruction> ins,
                            shared_ptr<Expression> op1,
                            shared_ptr<Expression> op2,
                            shared_ptr<Expression> op3,
                            const Location& l)
{
    hilti::instruction::Operands ops = { target, op1, op2, op3 };
    auto instr = std::make_shared<statement::instruction::Unresolved>(ins, ops, l);
    block->addStatement(instr);
}

void InstructionNormalizer::visit(Module* f)
{
}

void InstructionNormalizer::visit(declaration::Function* f)
{
    _names.clear();
}

void InstructionNormalizer::visit(statement::ForEach* s)
{
    shared_ptr<type::Reference> r = ast::as<type::Reference>(s->sequence()->type());
    shared_ptr<Type> t = r ? r->argType() : s->sequence()->type();
    auto iterable = ast::as<type::trait::Iterable>(t);

    if ( ! iterable )
        internalError(s, ::util::fmt("InstructionNormalizer/ForEach: %s is not iterable", t->render()));

    auto parent = s->firstParent<statement::Block>();
    assert(parent);

    auto nblock = std::make_shared<statement::Block>(nullptr);
    nblock->scope()->setParent(parent->scope());

    auto var = _addLocal(nblock, s->id()->name(), iterable->elementType(), true);
    auto end = _addLocal(nblock, "end",  iterable->iterType());
    auto iter = _addLocal(nblock, "iter", iterable->iterType());
    auto cmp = _addLocal(nblock, "cmp", std::make_shared<type::Bool>());

    auto entry = _addBlock(nblock, "loop_entry");
    auto cond = _addBlock(nblock, "loop_cond");
    auto deref = _addBlock(nblock, "loop_deref");
    auto body = _addBlock(nblock, "loop_body");
    auto next = _addBlock(nblock, "loop_next");
    auto cont = _addBlock(nblock, "loop_end");

    IDReplacer replacer;
    replacer.run(s->body(), std::make_shared<ID>(statement::foreach::IDLoopBreak), cont.first->id());
    replacer.run(s->body(), std::make_shared<ID>(statement::foreach::IDLoopNext), next.first->id());

    _addInstruction(entry.first, iter, instruction::operator_::Begin, s->sequence());
    _addInstruction(entry.first, end,  instruction::operator_::End, s->sequence());
    _addInstruction(entry.first, nullptr, instruction::flow::Jump, cond.second);

    _addInstruction(cond.first, cmp, instruction::operator_::Equal, iter, end);
    _addInstruction(cond.first, nullptr, instruction::flow::IfElse, cmp, cont.second, deref.second);

    _addInstruction(deref.first, var, instruction::operator_::Deref, iter);
    _addInstruction(deref.first, nullptr, instruction::flow::Jump, body.second);

    auto b = s->body();

    body.first->addStatement(b);
    b->scope()->setParent(body.first->scope());

    _addInstruction(next.first, iter, instruction::operator_::Incr, iter);
    _addInstruction(next.first, nullptr, instruction::flow::Jump, cond.second);

    s->replace(nblock);
}

void InstructionNormalizer::visit(statement::Try* s)
{
    auto parent = s->firstParent<statement::Block>();
    assert(parent);

    auto nblock = std::make_shared<statement::Block>(nullptr);
    nblock->scope()->setParent(parent->scope());

    auto cblocks = std::make_shared<statement::Block>(nullptr, nullptr);
    cblocks->scope()->setParent(nblock->scope());

    auto cont = _addBlock(nblock, "catch_cont", true);

    for ( auto i = s->catches().rbegin(); i != s->catches().rend(); i++ ) {
        auto c = *i;

        auto cblock = _addBlock(nblock, "catch", true);

        if ( c->type() ) {
            auto var = _addLocal(cblock.first, c->id()->name(), c->type(), true);
            _addInstruction(cblock.first, var, instruction::exception::__GetAndClear);
        }

        else
            _addInstruction(cblock.first, nullptr, instruction::exception::__Clear);

        c->block()->removeFromParents();
        cblock.first->addStatement(c->block());

        _addInstruction(cblock.first, nullptr, instruction::flow::Jump, cont.second);

        shared_ptr<Expression> type = nullptr;

        if ( c->type() ) {
            auto rtype = ast::tryCast<type::Reference>(c->type());

            if ( ! rtype ) {
                error(c->type(), "catch value must be of ref<exception> type");
                return;
            }

            type = builder::type::create(rtype->argType());
        }

        _addInstruction(nblock, nullptr, instruction::exception::__BeginHandler, cblock.second, type);
        cblocks->addStatement(cblock.first);
    }

    s->block()->removeFromParents();
    nblock->addStatement(s->block());

    for ( auto c : s->catches() )
        _addInstruction(nblock, nullptr, instruction::exception::__EndHandler);

    _addInstruction(nblock, nullptr, instruction::flow::Jump, cont.second);

    nblock->addStatement(cblocks);
    nblock->addStatement(cont.first);

    s->replace(nblock);
}


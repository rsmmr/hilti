
#include "hilti-intern.h"
#include "instructions/flow.h"

using namespace hilti::passes;

CFG::CFG(CompilerContext* context) : Pass<>("hilti::CFG")
{
    _context = context;
}

CFG::~CFG()
{
}

bool CFG::run(shared_ptr<Node> node)
{
    auto module = ast::checkedCast<Module>(node);

    _pass = 1;

    if ( module->body() && ! processOne(module->body()) )
        return false;

    _pass = 2;

    do {
        _changed = false;

        if ( module->body() && ! processAllPreOrder(module->body()) )
            return false;
    } while ( _changed );

    _pass = 0;

    _run = (errors() == 0);
    return _run;
}

shared_ptr<std::set<shared_ptr<Statement>>> CFG::predecessors(shared_ptr<Statement> stmt)
{
    assert(_run || _pass > 0);
    auto i = _predecessors.find(stmt);
    return i != _predecessors.end() ? i->second : std::make_shared<std::set<shared_ptr<Statement>>>();
}

shared_ptr<std::set<shared_ptr<Statement>>> CFG::successors(shared_ptr<Statement> stmt)
{
    assert(_run || _pass > 0);
    auto i = _successors.find(stmt);
    return i != _successors.end() ? i->second : std::make_shared<std::set<shared_ptr<Statement>>>();
}

void CFG::addSuccessor(Statement* stmt, shared_ptr<Statement> succ)
{
    addSuccessor(stmt->sharedPtr<Statement>(), succ);
}

void CFG::addSuccessor(shared_ptr<Statement> stmt, shared_ptr<Statement> succ)
{
    if ( ! (stmt && succ) )
        return;

    stmt = stmt->firstNonBlock();
    succ = succ->firstNonBlock();

    if ( ! (stmt && succ) )
        return;

    auto i = _successors.find(stmt);

    if ( i == _successors.end() ) {
        auto set = std::make_shared<std::set<shared_ptr<Statement>>>();
        auto j = _successors.insert(std::make_pair(stmt, set));
        i = j.first;
    }

    if ( i->second->find(succ) == i->second->end() ) {
        i->second->insert(succ);
        _changed = true;
    }

    i = _predecessors.find(succ);

    if ( i == _predecessors.end() ) {
        auto set = std::make_shared<std::set<shared_ptr<Statement>>>();
        auto j = _predecessors.insert(std::make_pair(succ, set));
        i = j.first;
    }

    if ( i->second->find(stmt) == i->second->end() ) {
        i->second->insert(stmt);
        _changed = true;
    }
}

void CFG::setSuccessors(shared_ptr<Statement> stmt, std::set<shared_ptr<Statement>> succs)
{
    auto i = _successors.find(stmt);

    if ( i == _successors.end() ) {
        auto set = std::make_shared<std::set<shared_ptr<Statement>>>();
        auto j = _successors.insert(std::make_pair(stmt, set));
        i = j.first;
    }

    auto old_set = *i->second;
    if ( old_set != succs ) {
        *i->second = succs;
        _changed = true;
    }
}

void CFG::setPredecessors(shared_ptr<Statement> stmt, std::set<shared_ptr<Statement>> preds)
{
    auto i = _predecessors.find(stmt);

    if ( i == _predecessors.end() ) {
        auto set = std::make_shared<std::set<shared_ptr<Statement>>>();
        auto j = _predecessors.insert(std::make_pair(stmt, set));
        i = j.first;
    }

    auto old_set = *i->second;
    if ( old_set != preds ) {
        *i->second = preds;
        _changed = true;
    }
}

void CFG::visit(declaration::Function* f)
{
    if ( _pass == 1 ) {
        auto body = f->function()->body();

        if ( body )
            processOne(body);
    }
}

void CFG::visit(statement::Block* s)
{
    if ( _pass == 1 ) {
        // Link the non-block statements.

        auto cur = s->statements().begin();
        auto next = cur;
        std::advance(next, 1);

        while ( cur != s->statements().end() ) {
            shared_ptr<Statement> next_instr = nullptr;

            if ( next != s->statements().end() )
                next_instr = (*next)->firstNonBlock();

            if ( ! next_instr && _siblings.size() )
                next_instr = _siblings.back();

            if ( next_instr ) {
                auto ins = ast::tryCast<statement::instruction::Resolved>(*cur);

                if ( ins && ! ins->instruction()->terminator() )
                    addSuccessor(*cur, next_instr);

                _siblings.push_back(next_instr);
            }

            processOne(*cur);

            if ( next_instr )
                _siblings.pop_back();

            ++cur;
            ++next;
        }

        for ( auto d : s->declarations() )
            processOne(d);
    }

#if 0
    if ( _pass == 2 ) {
        // Block chaining is disabled. With the structure of having
        // statements with subblocks it's not clear how the chaining should
        // work.

        // Link the blocks.
        std::set<shared_ptr<Statement>> succs;
        std::set<shared_ptr<Statement>> preds;

        for ( auto stmt : s->statements() ) {
            for ( auto t : *successors(stmt) ) {
                auto p = t->firstParent<statement::Block>();
                if ( p && p.get() != s && ! s->hasChild(p, true) )
                    succs.insert(p);
            }

            for ( auto t : *predecessors(stmt) ) {
                auto p = t->firstParent<statement::Block>();
                if ( p && p.get() != s && ! s->hasChild(p, true) )
                    preds.insert(p);
            }
        }

        setSuccessors(s->sharedPtr<Statement>(), succs);
        setPredecessors(s->sharedPtr<Statement>(), preds);
    }
#endif
}

void CFG::visit(statement::instruction::Resolved* s)
{
    if ( _pass == 1 ) {
        auto fi = s->flowInfo();

        for ( auto succ : fi.successors )
            addSuccessor(s, succ);

        if ( dynamic_cast<statement::instruction::flow::BlockEnd *>(s) && _siblings.size() ) {
            auto succ = _siblings.back();
            addSuccessor(s, succ);
        }
    }
}

void CFG::visit(statement::instruction::Unresolved* s)
{
    internalError("CFG::visit(statement::instruction::Unresolved* s) should not be reachable");
}

void CFG::visit(statement::Try* s)
{
    if ( _pass == 1 ) {
        std::set<shared_ptr<Statement>> catches;

        for ( auto c : s->catches() ) {
            auto i = c->block()->firstNonBlock();
            if ( i )
                catches.insert(i);

            processOne(c);
        }

        for ( auto child : s->block()->childs(true) ) {

            if ( ! ast::isA<Statement>(child) )
                continue;

            if ( ast::isA<statement::Block>(child) )
                continue;

            // TODO: This is unfortunate. As we don't know which instruction
            // can throw what kind of exception, we need to add egdes from
            // each to any catch handler. Eventually we should provide
            // exception information on a per instruction basis and then
            // remove limit the edges to what can actually happen.
            for ( auto c: s->catches() ) {
                auto i = c->block()->firstNonBlock();
                addSuccessor(child, i);
            }
        }

        auto i = s->block()->firstNonBlock();
        processOne(s->block());
        addSuccessor(s, i);
    }
}

void CFG::visit(statement::try_::Catch* s)
{
    if ( _pass == 1 ) {
        auto i = s->block()->firstNonBlock();
        processOne(s->block());
    }
}

void CFG::visit(statement::ForEach* s)
{
    if ( ! s->body() )
        return;

    if ( _pass == 1 ) {
        _siblings.push_back(s->sharedPtr<Statement>());
        processOne(s->body());
        _siblings.pop_back();

        auto i = s->body()->firstNonBlock();

        if ( i )
            addSuccessor(s, i);

        if ( _siblings.size() )
            addSuccessor(s, _siblings.back());
    }
}



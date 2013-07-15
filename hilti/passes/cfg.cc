
#include "hilti-intern.h"
#include "instructions/flow.h"
#include "instructions/exception.h"

using namespace hilti::passes;

// A helper pass that traverses nodes in depth-first order and records all
// statements it encounters.
class hilti::passes::DepthOrderTraversal : public Pass<>
{
public:
    DepthOrderTraversal(CFG* cfg) : Pass<>("hilti::cfg::DepthOrderTraversal") {
        _cfg = cfg;
    }

    virtual ~DepthOrderTraversal() {}

    bool run(shared_ptr<Node> stmt) override { processOne(stmt); _done.clear(); return true; }
    const std::list<shared_ptr<Statement>>& depthFirstOrder() const { return _statements; }

protected:
    void visit(Module* m) override {
        if ( m->body() )
            processOne(m->body());
    }

    void visit(declaration::Function* f) override {
        if ( f->function()->body() )
            processOne(f->function()->body());
    }

    void visit(Statement* s) override {
        auto stmt = s->sharedPtr<Statement>();

        if ( _done.find(stmt) != _done.end() )
            return;

        _done.insert(stmt);

        auto b = ast::tryCast<statement::Block>(stmt);

        if ( b ) {
            auto stmts = b->statements();

            for ( auto d : b->declarations() )
                processOne(d);

            if ( stmts.size() )
                processOne(stmts.front());
        }

        else {
            for ( auto p : _cfg->successors(stmt) )
                processOne(p);

            _statements.push_back(stmt);
        }
    }

#if 0
    void visit(statement::Block* s) override {
        auto stmt = s->sharedPtr<Statement>();

        if ( _done.find(stmt) != _done.end() )
            return;

        _done.insert(stmt);
    }
#endif

private:
    CFG* _cfg;
    std::list<shared_ptr<Statement>> _statements;
    std::set<shared_ptr<Statement>> _done;
};

CFG::CFG(CompilerContext* context) : Pass<>("hilti::CFG")
{
    _context = context;
    _dot = std::make_shared<DepthOrderTraversal>(this);
}

CFG::~CFG()
{
}

bool CFG::run(shared_ptr<Node> node)
{
    auto module = ast::checkedCast<Module>(node);

    if ( ! processAllPreOrder(node) )
        return false;

    _run = (errors() == 0);

    if ( _run )
        _dot->run(module);

    return _run;
}

std::set<shared_ptr<Statement>> CFG::predecessors(shared_ptr<Statement> stmt)
{
    assert(_run || _pass > 0);
    auto i = _predecessors.find(stmt);
    return i != _predecessors.end() ? i->second : std::set<shared_ptr<Statement>>();
}

std::set<shared_ptr<Statement>> CFG::successors(shared_ptr<Statement> stmt)
{
    assert(_run || _pass > 0);
    auto i = _successors.find(stmt);
    return i != _successors.end() ? i->second : std::set<shared_ptr<Statement>>();
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
        auto set = std::set<shared_ptr<Statement>>();
        auto j = _successors.insert(std::make_pair(stmt, set));
        i = j.first;
    }

    if ( i->second.find(succ) == i->second.end() ) {
        i->second.insert(succ);
        _changed = true;
    }

    i = _predecessors.find(succ);

    if ( i == _predecessors.end() ) {
        auto set = std::set<shared_ptr<Statement>>();
        auto j = _predecessors.insert(std::make_pair(succ, set));
        i = j.first;
    }

    if ( i->second.find(stmt) == i->second.end() ) {
        i->second.insert(stmt);
        _changed = true;
    }
}

void CFG::setSuccessors(shared_ptr<Statement> stmt, std::set<shared_ptr<Statement>> succs)
{
    auto i = _successors.find(stmt);

    if ( i == _successors.end() ) {
        auto set = std::set<shared_ptr<Statement>>();
        auto j = _successors.insert(std::make_pair(stmt, set));
        i = j.first;
    }

    auto old_set = i->second;
    if ( old_set != succs ) {
        i->second = succs;
        _changed = true;
    }
}

void CFG::setPredecessors(shared_ptr<Statement> stmt, std::set<shared_ptr<Statement>> preds)
{
    auto i = _predecessors.find(stmt);

    if ( i == _predecessors.end() ) {
        auto set = std::set<shared_ptr<Statement>>();
        auto j = _predecessors.insert(std::make_pair(stmt, set));
        i = j.first;
    }

    auto old_set = i->second;
    if ( old_set != preds ) {
        i->second = preds;
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

const std::list<shared_ptr<Statement>>& CFG::depthFirstOrder() const
{
    assert(_run);
    return _dot->depthFirstOrder();
}

void CFG::visit(statement::Block* s)
{
    // TODO: Track block flow.
}

void CFG::visit(statement::instruction::Resolved* s)
{
    auto fi = s->flowInfo();

    for ( auto succ : fi.successors )
        addSuccessor(s, succ);

    if ( s->successor() ) {
        assert(! s->instruction()->terminator());
        addSuccessor(s, s->successor());
    }

    for ( auto c : _excpt_handlers )
        addSuccessor(s, c.first->block());
}

void CFG::visit(statement::instruction::Unresolved* s)
{
    internalError("CFG::visit(statement::instruction::Unresolved* s) should not be reachable");
}

void CFG::visit(statement::instruction::exception::__BeginHandler* i)
{
    auto parent = i->firstParent<statement::Block>();
    assert(parent);

    auto const_ = ast::checkedCast<expression::Constant>(i->op1());
    auto label = ast::checkedCast<constant::Label>(const_->constant());
    auto eblock = parent->scope()->lookupUnique(std::make_shared<ID>(label->value()));
    assert(eblock);

    auto block = ast::checkedCast<expression::Block>(eblock);
    auto type = i->op2() ? ast::checkedCast<expression::Type>(i->op2())->typeValue() : nullptr;
    _excpt_handlers.push_back(std::make_pair(block, type));
}

void CFG::visit(statement::instruction::exception::__EndHandler* i)
{
    _excpt_handlers.pop_back();
}



// TODO: This is quite inefficient. We should traverse nodes in reverse CFG
// order (do a depth-first sorting of its nodes. Also, we should switch to
// "real" basic blocks by first turning try/catch, foreach, etc. into that
// structure so that we don't have nested blocks anymore. Then we can do
// the liveness on just blocks.

#include <functional>

#include "hilti-intern.h"

using namespace hilti::passes;

Liveness::Liveness(CompilerContext* context, shared_ptr<CFG> cfg) : Pass<>("hilti::Liveness")
{
    _context = context;
    _cfg = cfg;
}


Liveness::~Liveness()
{
}

static string renderSet(const Liveness::variable_set& vars)
{
    std::set<string> s;
    for ( auto v : vars )
        s.insert(v->name);

    return util::strjoin(s, ", ");
}

size_t Liveness::hashLiveness()
{
    uint64_t hash = 0;

    std::hash<std::shared_ptr<Statement>> hash_ptr;
    std::hash<std::string> hash_str;

    for ( auto i : _livenesses ) {
        auto stmt = i.first;
        auto sets = i.second;

        hash += hash_ptr(stmt);
        hash *= 17;

        for ( auto v : util::set_union(util::set_union(*sets.in, *sets.out), *sets.dead) ) {
            hash += hash_str(v->name);
            hash *= 17;
        }
    }

    return hash;
}

bool Liveness::run(shared_ptr<Node> block)
{
    size_t old_size;
    size_t old_hash;

    do  {
        old_size = _livenesses.size();
        old_hash = hashLiveness();

        for ( auto s : _cfg->depthFirstOrder() )
            processStatement(s);

        // std::cerr << "-> old_size=" << old_size << " new_size=" << _livenesses.size()
        //  << " old_hash=" << old_hash << " new_hash=" << hashLiveness() << std::endl;
        //  std::cerr << std::endl;

        // Note we can't track changes as we make them to the sets because
        // during on iteration a set might temporarily change but then revert
        // to the original value. Hence we need to compare the complete
        // original set with the complete result set; that's why we do the
        // expensive hashing.

    } while ( old_size != _livenesses.size() || old_hash != hashLiveness() );

    return (errors() == 0);
}

Liveness::LivenessSets Liveness::liveness(shared_ptr<Statement> stmt) const
{
    auto i = _livenesses.find(stmt);

    if ( i != _livenesses.end() )
        return i->second;

    auto empty = std::shared_ptr<variable_set>(new variable_set());

    LivenessSets sets;
    sets.in = sets.out = sets.dead = empty;
    return sets;
}

bool Liveness::have(shared_ptr<Statement> stmt) const
{
    return  _livenesses.find(stmt) != _livenesses.end();
}

bool Liveness::liveIn(shared_ptr<Statement> stmt, shared_ptr<hilti::type::function::Parameter> p) const
{
    auto block = ast::tryCast<statement::Block>(stmt);

    if ( block )
        stmt = block->firstNonBlock();

    auto ln = liveness(stmt).in;

    auto fv = std::make_shared<Statement::FlowVariable>(std::make_shared<expression::Parameter>(p));
    return ln->find(fv) != ln->end();
}

bool Liveness::liveIn(shared_ptr<Statement> stmt, shared_ptr<variable::Local> v) const
{
    auto block = ast::tryCast<statement::Block>(stmt);

    if ( block )
        stmt = block->firstNonBlock();

    auto ln = liveness(stmt).in;

    auto fv = std::make_shared<Statement::FlowVariable>(std::make_shared<expression::Variable>(v));
    return ln->find(fv) != ln->end();
}

bool Liveness::deadOut(shared_ptr<Statement> stmt, shared_ptr<Expression> e) const
{
    auto block = ast::tryCast<statement::Block>(stmt);

    if ( block )
        stmt = block->firstNonBlock();

    auto ln = liveness(stmt).dead;

    shared_ptr<Statement::FlowVariable> fv = nullptr;


    if ( auto ep = ast::tryCast<expression::Parameter>(e) )
        fv = std::make_shared<Statement::FlowVariable>(ep);

    else if ( auto ev = ast::tryCast<expression::Variable>(e) )
        fv = std::make_shared<Statement::FlowVariable>(ev);

    assert(fv);

    return ln->find(fv) != ln->end();
}

bool Liveness::liveOut(shared_ptr<Statement> stmt, shared_ptr<Expression> e) const
{
    auto block = ast::tryCast<statement::Block>(stmt);

    if ( block )
        stmt = block->firstNonBlock();

    auto ln = liveness(stmt).out;

    shared_ptr<Statement::FlowVariable> fv = nullptr;

    if ( auto ep = ast::tryCast<expression::Parameter>(e) )
        fv = std::make_shared<Statement::FlowVariable>(ep);

    else if ( auto ev = ast::tryCast<expression::Variable>(e) )
        fv = std::make_shared<Statement::FlowVariable>(ev);

    assert(fv);

    return ln->find(fv) != ln->end();
}

bool Liveness::liveIn(shared_ptr<Statement> stmt, shared_ptr<Expression> e) const
{
    auto block = ast::tryCast<statement::Block>(stmt);

    if ( block )
        stmt = block->firstNonBlock();

    auto ln = liveness(stmt).in;

    shared_ptr<Statement::FlowVariable> fv = nullptr;

    if ( auto ep = ast::tryCast<expression::Parameter>(e) )
        fv = std::make_shared<Statement::FlowVariable>(ep);

    else if ( auto ev = ast::tryCast<expression::Variable>(e) )
        fv = std::make_shared<Statement::FlowVariable>(ev);

    assert(fv);

    return ln->find(fv) != ln->end();
}

void Liveness::setLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out)
{
    auto i = _livenesses.find(stmt);

    if ( i == _livenesses.end() ) {
        LivenessSets sets;
        sets.in = std::make_shared<variable_set>();
        sets.out = std::make_shared<variable_set>();
        sets.dead = std::make_shared<variable_set>();
        auto j = _livenesses.insert(std::make_pair(stmt, sets));
        i = j.first;
    }

    auto sets = i->second;

    *sets.in = in;
    *sets.out = out;

    auto fi = stmt->flowInfo();

    *sets.dead = ::util::set_difference(in, out);
    *sets.dead = ::util::set_union(*sets.dead, ::util::set_difference(fi.defined, out));

    variable_set pred_live;
    for ( auto p : *_cfg->predecessors(stmt) )
        pred_live = ::util::set_union(pred_live, *liveness(p).out);

    pred_live = ::util::set_difference(pred_live, *sets.in);
    pred_live = ::util::set_difference(pred_live, *sets.out);

    *sets.dead = ::util::set_union(*sets.dead, pred_live);
}

void Liveness::addLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out)
{
    auto cur = liveness(stmt);

    for ( auto i : in )
        cur.in->insert(i);

    for ( auto i : out )
        cur.out->insert(i);
}

void Liveness::processStatement(shared_ptr<Statement> stmt)
{
    assert(! ast::tryCast<statement::Block>(stmt));

    variable_set in, out;

    for ( auto succ : *_cfg->successors(stmt) )
        out = util::set_union(out, *liveness(succ).in);

    auto fi = stmt->flowInfo();

    in = out;

    auto remove = util::set_union(fi.defined, fi.cleared);
    auto add = util::set_union(fi.modified, fi.read);

    if ( remove.size() )
        in = util::set_difference(in, remove);

    if ( add.size() )
        in = util::set_union(in, add);

    if ( fi.cleared.size() )
        out = util::set_difference(out, fi.cleared);

    setLiveness(stmt, in, out);
}




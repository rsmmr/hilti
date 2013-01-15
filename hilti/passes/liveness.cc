
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
        auto pair = i.second;

        hash += hash_ptr(stmt);
        hash *= 17;

        for ( auto v : util::set_union(*pair.first, *pair.second) ) {
            hash += hash_str(v->name);
            hash *= 17;
        }
    }

    return hash;
}

bool Liveness::run(shared_ptr<Node> module)
{
    size_t old_size;
    size_t old_hash;

    do  {
        old_size = _livenesses.size();
        old_hash = hashLiveness();

        // We do a reverse sibling traversal as that'll likely need less
        // iterations to converge.
        if ( ! processAllPreOrder(module, true) )
            return false;

        // std::cerr << "-> changed=" << _changed << " old_size=" << old_size << " new_size=" << _livenesses.size()
        // << " old_hash=" << old_hash << " new_hash=" << hashLiveness() << std::endl;
        // std::cerr << std::endl;

        // Note we can't track changes as we make them to the sets because
        // during on iteration a set might temporarily change but then revert
        // to the original value. Hence we need to compare the complete
        // original set with the complete result set; that's why we do the
        // expensive hashing.

    } while ( old_size != _livenesses.size() || old_hash != hashLiveness() );

    _run = (errors() == 0);
    return _run;
}

Liveness::liveness_sets Liveness::liveness(shared_ptr<Statement> stmt) const
{
    auto i = _livenesses.find(stmt);

    if ( i != _livenesses.end() )
        return i->second;

    auto empty = std::shared_ptr<variable_set>(new variable_set());
    return std::make_pair(empty, empty);
}

void Liveness::setLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out)
{
    auto i = _livenesses.find(stmt);

    if ( i == _livenesses.end() ) {
        auto ein = std::make_shared<variable_set>();
        auto eout = std::make_shared<variable_set>();
        auto j = _livenesses.insert(std::make_pair(stmt, std::make_pair(ein, eout)));
        i = j.first;
    }

    auto sets = i->second;

    if ( *sets.first != in ) {
        // std::cerr << stmt->render() << " IN : " << renderSet(*sets.first) << " -> " << renderSet(in) << std::endl;
        *sets.first = in;
    }

    if ( *sets.second != out ) {
        // std::cerr << stmt->render() << " OUT: " << renderSet(*sets.second) << " -> " << renderSet(out) << std::endl;
        *sets.second = out;
    }
}

void Liveness::addLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out)
{
    auto old = liveness(stmt);
    in = util::set_union(*old.first, in);
    out = util::set_union(*old.second, out);
    setLiveness(stmt, in, out);
}

void Liveness::visit(Statement* s)
{
    if ( ast::tryCast<statement::Block>(s) )
        return;

    auto stmt = s->sharedPtr<Statement>();

    variable_set in, out;

    for ( auto succ : *_cfg->successors(stmt) )
        out = util::set_union(out, *liveness(succ).first);

    auto fi = s->flowInfo();

    in = out;

    in = util::set_difference(in, fi.defined);
    in = util::set_difference(in, fi.cleared);
    in = util::set_union(in, fi.modified);
    in = util::set_union(in, fi.read);

    out = util::set_difference(out, fi.cleared);

    setLiveness(stmt, in, out);
}

void Liveness::visit(statement::Block* s)
{
}

void Liveness::visit(declaration::Function* s)
{
    if ( ! s->function()->body() )
        return;

    // For the first instruction of a function, add the parameters to IN.
    variable_set parameters;

    for ( auto p : s->function()->type()->parameters() ) {
        auto expr = std::make_shared<expression::Parameter>(p, p->location());
        parameters.insert(std::make_shared<Statement::FlowVariable>(expr));
    }

    if ( auto i = s->function()->body()->firstNonBlock() )
        addLiveness(i, parameters, variable_set());
}



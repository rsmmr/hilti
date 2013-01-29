
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

    *sets.first = in;
    *sets.second = out;
}

void Liveness::addLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out)
{
    auto cur = liveness(stmt);

    for ( auto i : in )
        cur.first->insert(i);

    for ( auto i : out )
        cur.second->insert(i);
}

void Liveness::processStatement(shared_ptr<Statement> stmt)
{
    assert(! ast::tryCast<statement::Block>(stmt));

    variable_set in, out;

    for ( auto succ : *_cfg->successors(stmt) )
        out = util::set_union(out, *liveness(succ).first);

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




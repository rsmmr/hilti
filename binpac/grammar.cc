
#include <vector>

#include <util/util.h>

#include "grammar.h"
#include "expression.h"

using namespace binpac;

Grammar::Grammar(const string& name, shared_ptr<Production> root, const parameter_list& params, const parameter_list& attrs, const Location& l)
{
    _name = name;
    _root = root;
    _params = params;
    _attrs = attrs;
    _location = l;
    _epsilon = std::make_shared<production::Epsilon>();

    if ( root ) {
        _addProduction(root);
        _simplify();
        _simplify();
        _computeTables();
    }
}

const string& Grammar::name() const
{
    return _name;
}

const Location& Grammar::location() const
{
    return _location;
}

const Grammar::parameter_list& Grammar::parameters() const
{
    return _params;
}

string Grammar::_productionLocation(shared_ptr<Production> p) const
{
    string gloc, gname, ploc, pname;

    if ( _location != Location::None )
        gloc = util::fmt(" (%s)", string(_location).c_str());

    if ( p->location() != Location::None )
        ploc = util::fmt(" (%s)", string(p->location()).c_str());

    if ( _name.size() )
        gname = util::fmt("grammar %s%s, ", _name.c_str(), gloc.c_str());

    pname = util::fmt("production %s%s", p->symbol().c_str(), ploc.c_str());

    return util::fmt("%s%s", gname.c_str(), pname.c_str());
}

string Grammar::check() const
{
    string msg;

    for ( auto p : _lah_errors )
        msg += util::fmt("%s: look-ahead cannot depend on non-terminal\n", _productionLocation(p).c_str());

    for ( auto p : _nterms ) {
        auto lap = std::dynamic_pointer_cast<production::LookAhead>(p);

        if ( ! lap )
            continue;

        auto laheads = lap->lookAheads();

        std::set<string> syms1, syms2;

        for ( auto p : laheads.first )
            syms1.insert(util::fmt("%s (%s)", p->renderTerminal(), p->type()->render()));

        for ( auto p : laheads.second )
            syms2.insert(util::fmt("%s (%s)", p->renderTerminal(), p->type()->render()));

        auto defaults = lap->defaultAlternatives();

        if ( defaults.first && defaults.second ) {
            msg += util::fmt("%s: no look-ahead token for either alternative\n", _productionLocation(p).c_str());
            break;
        }

        if ( syms1.size() == 0 && syms2.size() == 0 ) {
            msg += util::fmt("no look-ahead symbol for either alternative in %s\n", _productionLocation(lap));
            break;
        }

        auto isect = util::set_intersection(syms1, syms2);

        if ( isect.size() )
            msg += util::fmt("%s is ambigious for look-ahead symbol(s) { %s }\n", _productionLocation(lap).c_str(), util::strjoin(isect, ", ").c_str());

        for ( auto q : util::set_union(laheads.first, laheads.second) ) {
            if ( ! std::dynamic_pointer_cast<production::Terminal>(q) )
                msg += util::fmt("%s: look-ahead cannot depend on non-terminal\n", _productionLocation(p).c_str());
                break;
        }
    }

    return msg;
}

const Grammar::production_map& Grammar::productions() const
{
    return _prods;
}

shared_ptr<Production> Grammar::root() const
{
    return _root;
}

string _fmtProds(std::set<string> prods)
{
    return util::strjoin(prods, ", ");
}

void Grammar::printTables(std::ostream& out, bool verbose)
{
    out << "=== Grammar " << _name << std::endl;

    for ( auto i : _prods ) {
        auto sym = i.first;
        auto prod = i.second;
        auto start = (prod == _root ? "(*)" : "");

        out << util::fmt(" %3s %s", start, prod->render().c_str()) << std::endl;
    }

    if ( ! verbose ) {
        out << std::endl;
        return;
    }

    out << std::endl;
    out << "  -- Epsilon:" << std::endl;

    for ( auto i : _nullable )
        out << "     " << i.first.c_str() << " = " << i.second << std::endl;

    out << std::endl;
    out << "  -- First_1:" << std::endl;

    for ( auto i : _first )
        out << "     " << i.first.c_str() << " = { " << _fmtProds(i.second).c_str() << " }" << std::endl;

    out << std::endl;
    out << "  -- Follow:" << std::endl;

    for ( auto i : _follow )
        out << "     " << i.first.c_str() << " = { " << _fmtProds(i.second).c_str() << " }" << std::endl;

    out << std::endl;
}

void Grammar::_addProduction(shared_ptr<Production> p)
{
    auto epsilon = std::dynamic_pointer_cast<production::Epsilon>(p);

    if ( epsilon ) {
        // We use our own internal instance for all epsilon productions.
        p = _epsilon;
    }

    if ( _prods_set.find(p) != _prods_set.end() )
        // Already added this.
        return;

    // Create a unique symbol.
    int cnt = 1;
    auto sym = p->symbol();

    while ( _prods.find(sym) != _prods.end() ) {
        sym = util::fmt("%s.%d", p->symbol(), ++cnt);
    }

    p->setSymbol(sym);
    _prods.insert(std::make_pair(sym, p));
    _prods_set.insert(p);

    auto nterm = std::dynamic_pointer_cast<production::NonTerminal>(p);

    if ( nterm ) {
        _nterms.push_back(nterm);

        for ( auto rhss : nterm->rhss() )
            for ( auto rhs : rhss )
                _addProduction(rhs);
    }
}

void Grammar::_computeClosure(shared_ptr<Production> root, std::set<string>* used)
{
    auto sym = root->symbol();

    if ( used->find(sym) != used->end() )
        return;

    used->insert(sym);

    auto nterm = std::dynamic_pointer_cast<production::NonTerminal>(root);

    if ( ! nterm )
        return;

    for ( auto rhss : nterm->rhss() )
        for ( auto rhs : rhss )
            _computeClosure(rhs, used);
}

void Grammar::_simplify()
{
    // Remove unused productions.

    std::set<string> used;
    _computeClosure(_root, &used);

    for ( auto sym : util::set_difference(util::map_keys(_prods), used) ) {
        _prods.erase(sym);

        for ( auto i = _nterms.begin(); i != _nterms.end(); i++ ) {
            if ( (*i)->symbol() == sym ) {
                _nterms.erase(i);
                break;
            }
        }
    }

    // Do production-specific simplyfing.
    //
    //
    for ( auto p : _prods )
        p.second->simplify();
}

bool Grammar::_add(std::map<string, symbol_set>* tbl, shared_ptr<Production> dst, const symbol_set& src, bool changed)
{
    auto idx = dst->symbol();
    auto t = tbl->find(idx);
    assert(t != tbl->end());

    auto set = t->second;
    auto union_ = util::set_union(set, src);

    if ( union_.size() == set.size() )
        // All in there already.
        return changed;

    (*tbl)[idx] = union_;

    return true;
}

bool Grammar::_isNullable(production_list::iterator i, production_list::iterator j)
{
    while ( i != j ) {
        auto rhs = *i++;

        if ( std::dynamic_pointer_cast<production::Epsilon>(rhs) )
            continue;

        if ( std::dynamic_pointer_cast<production::Terminal>(rhs) )
            return false;

        if ( ! _nullable[rhs->symbol()] )
            return false;
    }

    return true;
}

Grammar::symbol_set Grammar::_getFirst(shared_ptr<Production> p)
{
    if ( std::dynamic_pointer_cast<production::Epsilon>(p) )
        return symbol_set();

    if ( std::dynamic_pointer_cast<production::Terminal>(p) )
        return { p->symbol() };

    return _first[p->symbol()];
}

Grammar::symbol_set Grammar::_getFirstOfRhs(production_list rhs)
{
    auto first = symbol_set();

    for ( auto p : rhs ) {
        if ( std::dynamic_pointer_cast<production::Epsilon>(p) )
            continue;

        if ( std::dynamic_pointer_cast<production::Terminal>(p) )
            return { p->symbol() };

        first = util::set_union(first, _first[p->symbol()]);

        if ( ! _nullable[p->symbol()] )
            break;
    }

    return first;
}

void Grammar::_computeTables()
{
    // Computes FIRST, FOLLOW, & NULLABLE. This follows roughly the Algorithm
    // 3.13 from Modern Compiler Implementation in C by Appel/Ginsburg. See
    // http://books.google.com/books?id=A3yqQuLW5RsC&pg=PA49.

    // Initializde sets.
    for ( auto p : _nterms ) {
        auto sym = p->symbol();
        _nullable[sym] = false;
        _first[sym] = symbol_set();
        _follow[sym] = symbol_set();
    }

    // Iterator until no further change.
    while ( true ) {
        bool changed = false;

        for ( auto p : _nterms ) {
            auto sym = p->symbol();

            for ( auto rhss : p->rhss() ) {
                auto first = rhss.begin();
                auto last = rhss.end();

                if ( _isNullable(first, last) && ! _nullable[sym] ) {
                    _nullable[sym] = true;
                    changed = true;
                }

                for ( auto i = first; i != last; i++ ) {
                    auto rhs = *i;

                    if ( _isNullable(first, i) )
                        changed = _add(&_first, p, _getFirst(rhs), changed);

                    if ( ! std::dynamic_pointer_cast<production::NonTerminal>(rhs) )
                        continue;

                    auto next = i;
                    ++next;

                    if ( _isNullable(next, last) )
                        changed = _add(&_follow, rhs, _follow[sym], changed);

                    for ( auto j = next; j != last; ++j ) {
                        if ( _isNullable(next, j) ) {
                            changed = _add(&_follow, rhs, _getFirst(*j), changed);
                        }
                    }
                }
            }
        }

        if ( ! changed )
            break;
    }

    // Build the look-ahead sets.

    for ( auto p : _nterms ) {
        auto sym = p->symbol();
        auto lap = std::dynamic_pointer_cast<production::LookAhead>(p);

        if ( ! lap )
            continue;

        auto rhss = p->rhss();

        assert(rhss.size() == 2);
        auto r = rhss.begin();

        std::vector<symbol_set> laheads = { symbol_set(), symbol_set() };

        for ( auto i = 0; i < 2; ++i ) {
            auto rhs = *r++;

            for ( auto term : _getFirstOfRhs(rhs) )
                laheads[i] = util::set_union(laheads[i], { term });

            if ( _isNullable(rhs.begin(), rhs.end()) ) {
                for ( auto term : _follow[sym] )
                    laheads[i] = util::set_union(laheads[i], { term });
            }

        }

        production::LookAhead::look_aheads v0;
        production::LookAhead::look_aheads v1;

        std::set<string> lahs;

        for ( auto i = 0; i < 2; ++i ) {
            for ( auto s : laheads[i] ) {
                auto p = _prods.find(s);
                assert(p != _prods.end());
                auto t = std::dynamic_pointer_cast<production::Terminal>(p->second);

                if ( ! t ) {
                    _lah_errors.push_back(p->second);
                    break;
                }

                if ( i == 0 )
                    v0.insert(t);
                else
                    v1.insert(t);
            }
        }

        lap->setLookAheads(v0, v1);
    }
}

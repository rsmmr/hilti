
#include <util/util.h>

#include "grammar-builder.h"
#include "declaration.h"
#include "type.h"
#include "grammar.h"
#include "production.h"

using namespace binpac;
using namespace binpac::passes;

GrammarBuilder::GrammarBuilder(std::ostream& out)
    : ast::Pass<AstInfo, shared_ptr<Production>>("GrammarBuilder"), _debug_out(out)
{
}

GrammarBuilder::~GrammarBuilder()
{
}

bool GrammarBuilder::run(shared_ptr<ast::NodeBase> ast)
{
    _in_decl = false;
    _counters.clear();
    return processAllPreOrder(ast);
}

void GrammarBuilder::enableDebug()
{
    _debug = true;
}

shared_ptr<Production> GrammarBuilder::compileOne(shared_ptr<Node> n)
{
    shared_ptr<Production> production = nullptr;
    bool success = processOne(n, &production);
    assert(success);
    assert(production);
    return production;
}

string GrammarBuilder::counter(const string& key)
{
    auto cnt = 1;
    auto i = _counters.find(key);

    if ( i != _counters.end() )
        cnt = i->second;

    string s = util::fmt("%s%d", key.c_str(), cnt++);

    _counters[key] = cnt;

    return s;
}

void GrammarBuilder::visit(declaration::Type* d)
{
    // We are only interested in unit declarations.
    auto unit = ast::tryCast<type::Unit>(d->type());

    if ( ! unit )
        return;

    _in_decl = true;
    auto production = compileOne(unit);
    _in_decl = false;

    auto grammar = std::make_shared<Grammar>(d->id()->name(), production);

    if ( _debug )
        grammar->printTables(_debug_out);

    unit->setGrammar(grammar);
}

void GrammarBuilder::visit(type::Unit* u)
{
    if ( ! _in_decl )
        return;

    auto prods = Production::production_list();

    for ( auto f : u->fields() )
        prods.push_back(compileOne(f));

    string name;

    if ( u->id() )
        name = u->id()->name();
    else
        name = util::fmt("unit%d", _unit_counter++);

    auto unit = std::make_shared<production::Sequence>(name, prods, u->sharedPtr<type::Unit>(), u->location());
    setResult(unit);
}

void GrammarBuilder::visit(type::unit::item::field::Constant* c)
{
    if ( ! _in_decl )
        return;
}

void GrammarBuilder::visit(type::unit::item::field::RegExp* r)
{
    if ( ! _in_decl )
        return;
}

void GrammarBuilder::visit(type::unit::item::field::Switch* s)
{
    if ( ! _in_decl )
        return;
}

void GrammarBuilder::visit(type::unit::item::field::Type* t)
{
    if ( ! _in_decl )
        return;

    auto sym = "var:" + t->id()->name();
    auto prod = std::make_shared<production::Variable>(sym, t->type());
    prod->pgMeta()->field = t->sharedPtr<type::unit::item::Field>();
    setResult(prod);
}

void GrammarBuilder::visit(type::unit::item::field::switch_::Case* c)
{
    if ( ! _in_decl )
        return;
}

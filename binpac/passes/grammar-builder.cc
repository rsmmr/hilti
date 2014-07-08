
#include <util/util.h>

#include "grammar-builder.h"
#include "../declaration.h"
#include "../type.h"
#include "../grammar.h"
#include "../production.h"
#include "../attribute.h"
#include "../expression.h"
#include "../constant.h"

using namespace binpac;
using namespace binpac::passes;

GrammarBuilder::GrammarBuilder(std::ostream& out)
    : ast::Pass<AstInfo, shared_ptr<Production>>("binpac::GrammarBuilder", false), _debug_out(out)
{
}

GrammarBuilder::~GrammarBuilder()
{
}

bool GrammarBuilder::run(shared_ptr<ast::NodeBase> ast)
{
    _in_decl = 0;;
    _counters.clear();
    return processAllPreOrder(ast);
}

void GrammarBuilder::enableDebug()
{
    _debug = true;
}

GrammarBuilder::production_map GrammarBuilder::_compiled;

shared_ptr<Production> GrammarBuilder::compileOne(shared_ptr<Node> n)
{
    auto p = _compiled.find(n);

    if ( p != _compiled.end() )
        return p->second;

    _compiled.insert(std::make_pair(n, std::make_shared<production::Unknown>(n)));

    shared_ptr<Production> production = nullptr;
    bool success = processOne(n, &production);
    assert(success);
    assert(production);

    _compiled.erase(n);
    _compiled.insert(std::make_pair(n, production));

    return production;
}

void GrammarBuilder::_resolveUnknown(shared_ptr<Production> production)
{
    auto unknown = ast::tryCast<production::Unknown>(production);

    if ( unknown ) {
        auto n = unknown->node();
        auto p = _compiled.find(n);
        assert( p != _compiled.end());
        production->replace(n);
    }

    for ( auto c : production->childs() )
        _resolveUnknown(c->sharedPtr<binpac::Production>());
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

    ++_in_decl;
    auto production = compileOne(unit);
    --_in_decl;

    _resolveUnknown(production);

    auto grammar = std::make_shared<Grammar>(d->id()->name(), production);

    if ( _debug )
        grammar->printTables(_debug_out, true);

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

    auto sym = "const:" + c->id()->name();
    auto prod = std::make_shared<production::Constant>(sym, c->constant());
    prod->pgMeta()->field = c->sharedPtr<type::unit::item::Field>();
    setResult(prod);
}

void GrammarBuilder::visit(type::unit::item::field::Ctor* c)
{
    if ( ! _in_decl )
        return;

    auto sym = "ctor:" + c->id()->name();
    auto prod = std::make_shared<production::Ctor>(sym, c->ctor());
    prod->pgMeta()->field = c->sharedPtr<type::unit::item::Field>();
    setResult(prod);
}

void GrammarBuilder::visit(type::unit::item::field::Switch* s)
{
    if ( ! _in_decl )
        return;

    production::Switch::case_list cases;
    shared_ptr<Production> default_ = nullptr;

    for ( auto c : s->cases() ) {
        if ( c->default_() ) {
            default_ = compileOne(c);
            continue;
        }

        auto pcase = std::make_pair(c->expressions(), compileOne(c));
        cases.push_back(pcase);
    }

    auto sym = "switch:" + s->id()->name();
    auto prod = std::make_shared<production::Switch>(sym, s->expression(), cases, default_, s->location());
    prod->pgMeta()->field = s->sharedPtr<type::unit::item::Field>();
    setResult(prod);
}

void GrammarBuilder::visit(type::unit::item::field::AtomicType* t)
{
    if ( ! _in_decl )
        return;

    shared_ptr<Production> prod;

    if ( ast::isA<type::EmbeddedObject>(t->type()) ) {
        // Not quite clear if there's a nicer way to present these than
        // special-casing them here.
        auto sym = "type:" + t->id()->name();
        prod = std::make_shared<production::TypeLiteral>(sym, t->type());
    }

    else {
        auto sym = "var:" + t->id()->name();
        prod = std::make_shared<production::Variable>(sym, t->type());
    }

    prod->pgMeta()->field = t->sharedPtr<type::unit::item::Field>();
    setResult(prod);
}

void GrammarBuilder::visit(type::unit::item::field::Unit* u)
{
    if ( ! _in_decl )
        return;

    ++_in_decl;
    auto chprod = compileOne(u->type());
    --_in_decl;

    string name;

    if ( u->id() )
        name = u->id()->name();
    else
        name = util::fmt("unit%d", _unit_counter++);

    auto child = std::make_shared<production::ChildGrammar>(name, chprod, ast::checkedCast<type::Unit>(u->type()), u->location());
    child->pgMeta()->field = u->sharedPtr<type::unit::item::field::Unit>();
    setResult(child);
}

void GrammarBuilder::visit(type::unit::item::field::switch_::Case* c)
{
    if ( ! _in_decl )
        return;

    auto prods = Production::production_list();

    for ( auto i : c->items() )
        prods.push_back(compileOne(i));

    auto name = util::fmt("case%d", _case_counter++);
    auto seq = std::make_shared<production::Sequence>(name, prods, nullptr, c->location());
    setResult(seq);
}

void GrammarBuilder::visit(type::unit::item::field::container::List* l)
{
    if ( ! _in_decl )
        return;

    auto until = l->attributes()->lookup("until");
    auto until_including = l->attributes()->lookup("until_including");
    auto while_ = l->attributes()->lookup("while");
    auto count = l->attributes()->lookup("count");
    auto length = l->attributes()->lookup("length");

    auto sym = "list:" + l->id()->name();

    ++_in_decl;
    auto field = compileOne(l->field());
    field->setContainer(l->sharedPtr<type::unit::item::field::Container>());
    --_in_decl;

    if ( until || while_ || until_including ) {
        // We use a Loop production here. type::Container installs a &foreach
        // hook that stops the iteration once the condition is satisfied.
        // Doing it this way allows the condition to run in the hook's scope,
        // with access to "$$".
        auto l1 = std::make_shared<production::Loop>(sym, field, false, l->location());
        l1->pgMeta()->field = l->sharedPtr<type::unit::item::Field>();
        setResult(l1);
    }

    else if ( count ) {
        auto l1 = std::make_shared<production::Counter>(sym, count->value(), field, l->location());
        l1->pgMeta()->field = l->sharedPtr<type::unit::item::Field>();
        setResult(l1);
    }

    else if ( length ) {
        auto l1 = std::make_shared<production::Loop>(sym, field, true, l->location());
        auto l2 = std::make_shared<production::ByteBlock>(sym, length->value(), l1, l->location());
        l1->pgMeta()->field = l->sharedPtr<type::unit::item::Field>();
        setResult(l2);
    }

    else {
        // No attributes, use look-ahead to figure out when to stop parsing.
        //
        // Left-factored & right-recursive.
        //
        // List1 -> Item List2
        // List2 -> Epsilon | List1

        auto epsilon = std::make_shared<production::Epsilon>(l->location());
        auto l1 = std::make_shared<production::Sequence>(sym + ":l1", Production::production_list(), nullptr, l->location());
        auto l2 = std::make_shared<production::LookAhead>(sym + ":l2", epsilon, l1, l->location());

        l1->add(field);
        l1->add(l2);

        auto c = std::make_shared<production::Enclosure>(sym, l2, l->location());
        c->pgMeta()->field = l->sharedPtr<type::unit::item::Field>();
        field->pgMeta()->for_each = c->pgMeta()->field;

        setResult(c);
    }
}

void GrammarBuilder::visit(type::unit::item::field::container::Vector* v)
{
    if ( ! _in_decl )
        return;

    auto length = v->length();
    auto sym = "vector:" + v->id()->name();

    ++_in_decl;
    auto field = compileOne(v->field());
    field->setContainer(v->sharedPtr<type::unit::item::field::Container>());
    --_in_decl;

    auto l1 = std::make_shared<production::Counter>(sym, v->length(), field, v->location());
    l1->pgMeta()->field = v->sharedPtr<type::unit::item::Field>();
    setResult(l1);
}

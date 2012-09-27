
#include "id-resolver.h"

#include "attribute.h"
#include "declaration.h"
#include "expression.h"
#include "scope.h"
#include "statement.h"
#include "type.h"

using namespace binpac;
using namespace binpac::passes;

IDResolver::IDResolver() : Pass<AstInfo>("IDResolver")
{
}

IDResolver::~IDResolver()
{
}

bool IDResolver::run(shared_ptr<ast::NodeBase> module)
{
    return processAllPreOrder(module);
}

void IDResolver::visit(expression::ID* i)
{
    // Find the nearest to scope.
    shared_ptr<Scope> scope = nullptr;

    auto nodes = currentNodes();

    for ( auto i = nodes.rbegin(); i != nodes.rend(); i++ ) {
        auto n = *i;

        auto unit = ast::tryCast<type::Unit>(n);
        auto block = ast::tryCast<statement::Block>(n);

        if ( block ) {
            scope = block->scope();
            break;
        }

        if ( unit ) {
            scope = unit->scope();
            break;
        }
    }

    if ( ! scope ) {
        error(i, "ID expression outside of any scope");
        return;
    }

    auto id = i->sharedPtr<expression::ID>();
    auto val = scope->lookup(id->id());

    if ( ! val ) {
        error(i, util::fmt("unknown ID %s", id->id()->pathAsString().c_str()));
        return;
    }

    id->replace(val);

    if ( id->id()->isScoped() )
        val->setScope(id->id()->scope());
}

void IDResolver::visit(type::Unknown* t)
{
    auto body = current<statement::Block>();

    if ( ! body ) {
        error(t, "ID expression outside of any scope");
        return;
    }

    auto id = t->id();

    if ( ! id )
        return;

    auto val = body->scope()->lookup(id);

    if ( ! val ) {
        error(t, util::fmt("unknown type ID %s", id->pathAsString().c_str()));
        return;
    }

    auto nt = ast::tryCast<expression::Type>(val);

    if ( ! nt ) {
        error(t, util::fmt("ID %s does not reference a type", id->pathAsString().c_str()));
        return;
    }

    auto tv = nt->typeValue();
    t->replace(tv);

    if ( ! tv->id() )
        tv->setID(id);
}

void IDResolver::visit(declaration::Hook* h)
{
    // Determine the unit that we belong to. For a path "a::b::c::, "a::b"
    // must reference its type.
    auto path = h->id()->path();
    path.pop_back();

    if ( ! path.size() ) {
        error(h, "id does not reference a hook field");
        return;
    }

    auto module = current<Module>();
    assert(module);

    auto expr = module->body()->scope()->lookup(std::make_shared<ID>(path));

    if ( ! expr ) {
        error(h, "id does not lead to a unit type");
        return;
    }

    if ( ! expr ) {
        error(h, "unknown id");
        return;
    }

    auto texpr = ast::tryCast<expression::Type>(expr);

    if ( ! texpr ) {
        error(h, "id does not reference a type");
        return;
    }

    auto unit = ast::tryCast<type::Unit>(texpr->typeValue());

    if ( ! unit ) {
        error(h, "id does not lead to a unit type");
        return;
    }

    // It's important here to run pre-order: setting the scope here links
    // the hook's scope to the unit's before descending down into the
    // statement.
    h->hook()->setUnit(unit);
}

void IDResolver::visit(type::unit::item::field::Unknown* f)
{
    auto body = current<statement::Block>();

    if ( ! body ) {
        error(f, "ID expression outside of any scope");
        return;
    }

    auto id = f->scopeID();

    if ( ! id )
        return;

    auto expr = body->scope()->lookup(id);

    if ( ! expr ) {
        error(f, util::fmt("unknown ID %s", id->pathAsString()));
        return;
    }

    auto attributes = f->attributes()->attributes();
    auto condition = f->condition();
    auto hooks = f->hooks();
    auto name = f->id();
    auto location = f->location();
    auto params = f->parameters();
    auto sinks = f->sinks();

    shared_ptr<type::unit::item::Field> nfield = nullptr;

    auto ctor = ast::tryCast<expression::Ctor>(expr);
    auto constant = ast::tryCast<expression::Constant>(expr);
    auto type = ast::tryCast<expression::Type>(expr);

    if ( ctor )
        nfield = std::make_shared<type::unit::item::field::Ctor>(name, ctor->ctor(), condition, hooks, attributes, sinks, location);

    if ( constant )
        nfield = std::make_shared<type::unit::item::field::Constant>(name, constant->constant(), condition, hooks, attributes, sinks, location);

    if ( type ) {
        auto tval = type->typeValue();
        auto unit = ast::tryCast<type::Unit>(tval);

        if ( unit )
            nfield = std::make_shared<type::unit::item::field::Unit>(name, unit, condition, hooks, attributes, params, sinks, location);
        else
            nfield = std::make_shared<type::unit::item::field::AtomicType>(name, tval, condition, hooks, attributes, sinks, location);
    }

    assert(nfield);
    f->replace(nfield);
}

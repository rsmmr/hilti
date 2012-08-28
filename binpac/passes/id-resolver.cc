
#include "id-resolver.h"

#include "declaration.h"
#include "expression.h"
#include "statement.h"
#include "scope.h"
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
    auto body = current<statement::Block>();

    if ( ! body ) {
        error(i, "ID expression outside of any scope");
        return;
    }

    auto id = i->sharedPtr<expression::ID>();
    auto val = body->scope()->lookup(id->id());

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

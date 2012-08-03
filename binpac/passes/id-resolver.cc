
#include "id-resolver.h"

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

    i->parent()->replaceChild(id, val);

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

    assert(t->parent());

    t->parent()->replaceChild(t->sharedPtr<binpac::Type>(), tv);

    if ( ! tv->id() )
        tv->setID(id);
}

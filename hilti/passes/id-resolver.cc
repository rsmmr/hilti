
#include "hilti.h"

using namespace hilti::passes;

IdResolver::~IdResolver()
{
}

void IdResolver::visit(expression::ID* i)
{
    auto body = current<statement::Block>();

    if ( ! body ) {
        error(i, "ID expression outside of any scope");
        return;
    }

    auto id = i->sharedPtr<expression::ID>();
    auto val = body->scope()->lookup(id->id());

    if ( ! val ) {
        error(i, util::fmt("unknown ID %s", id->id()->name().c_str()));
        return;
    }

    i->parent()->replaceChild(id, val);

    if ( id->id()->isScoped() )
        val->setScope(id->id()->scope());

    // TODO: IDs outside of expressions.
}

void IdResolver::visit(Declaration* d)
{
    auto module = current<Module>();
    assert(module);

    if ( module->exported(d->id()) )
        d->setExported(d);
}

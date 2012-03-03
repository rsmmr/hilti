
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
}

void IdResolver::visit(Declaration* d)
{
    auto module = current<Module>();
    assert(module);

    if ( module->exported(d->id()) )
        d->setExported(d);
}

void IdResolver::visit(type::Unknown* t)
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

    auto nt = ast::as<expression::Type>(val);

    if ( ! nt ) {
        error(t, util::fmt("ID %s does not reference a type", id->pathAsString().c_str()));
        return;
    }

    auto tv = nt->typeValue();

    assert(t->parent());

    t->parent()->replaceChild(t->sharedPtr<hilti::Type>(), tv);

    if ( ! tv->id() )
        tv->setID(id);
}

void IdResolver::visit(Function* f)
{
    _locals.clear();
}

void IdResolver::visit(variable::Local* v)
{
    // Assign unique intername names to all local variables.

    int cnt = 1;
    auto name = v->id()->name();

    while ( 1 ) {
        if ( _locals.find(name) == _locals.end() )
            break;

        name = util::fmt("%s.%d", v->id()->name().c_str(), ++cnt);
    };

    v->setInternalName(name);
    _locals.insert(name);
}


#include "hilti/hilti-intern.h"

using namespace hilti::passes;

IdResolver::~IdResolver()
{
}

bool IdResolver::run(shared_ptr<Node> module, bool report_unresolved)
{
    _report_unresolved = report_unresolved;
    return processAllPreOrder(module);
}

void IdResolver::visit(expression::ID* i)
{
    auto body = current<statement::Block>();

    if ( ! body ) {
        error(i, "ID expression outside of any scope");
        return;
    }

    auto id = i->sharedPtr<expression::ID>();
    auto vals = body->scope()->lookup(id->id());

    if ( ! vals.size() ) {
        if ( _report_unresolved )
            error(i, util::fmt("unknown ID %s", id->id()->pathAsString().c_str()));

        return;
    }

    if ( vals.size() > 1 ) {
        // This is ok if all hooks or one of them is scoped.
        for ( auto e : vals ) {
            auto func = ast::tryCast<expression::Function>(e);

            if ( func && ast::isA<Hook>(func->function()) )
                continue;

            if ( id->id()->isScoped() )
                break;

            error(i, util::fmt("ID %s defined more than once", id->id()->pathAsString().c_str()));
            return;
        }
    }

    auto val = vals.front();

    id->replace(val);

    if ( id->id()->isScoped() )
        val->setScope(id->id()->scope());
}

void IdResolver::visit(Declaration* d)
{
    auto module = current<Module>();
    assert(module);

    if ( module->exported(d->id()) )
        d->setLinkage(Declaration::EXPORTED);
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

    auto vals = body->scope()->lookup(id);

    if ( ! vals.size() ) {
        if ( _report_unresolved )
            error(t, util::fmt("unknown type ID %s", id->pathAsString().c_str()));

        return;
    }

    if ( vals.size() > 1 ) {
        error(t, util::fmt("ID %s defined more than once", id->pathAsString().c_str()));
        return;
    }

    auto val = vals.front();

    auto nt = ast::as<expression::Type>(val);

    if ( ! nt ) {
        if ( _report_unresolved )
            error(t, util::fmt("ID %s does not reference a type", id->pathAsString().c_str()));

        return;
    }

    auto tv = nt->typeValue();

    t->replace(tv);

    if ( tv->id() ) {
        if ( ! tv->id()->isScoped() )
            tv->setID(id);
    }
    else
        tv->setID(id);
}

void IdResolver::visit(Function* f)
{
}

void IdResolver::visit(declaration::Variable* d)
{
    // Assign unique intername names to all local variables.

    auto v = ast::tryCast<variable::Local>(d->variable());
    if ( ! v )
        return;

    auto func = current<Function>();
    if ( ! func )
        return;

    int cnt = 1;

    auto base = ::util::fmt("%s%%%s", func->id()->name(), v->id()->name());
    auto name = base;

    while ( 1 ) {
        if ( _locals.find(name) == _locals.end() )
            break;

        name = util::fmt("%s.%d", base, ++cnt);
    };

    v->setInternalName(name);
    _locals.insert(name);
}

void IdResolver::visit(statement::ForEach* s)
{
#if 0
    // Make sure the sequence type is reseolved.
    preOrder(s->sequence());

    shared_ptr<type::Reference> r = ast::as<type::Reference>(s->sequence()->type());
    shared_ptr<Type> t = r ? r->argType() : s->sequence()->type();

    auto iterable = ast::as<type::trait::Iterable>(t);

    if ( iterable && ! s->body()->scope()->has(s->id(), false) ) {
        auto var = std::make_shared<variable::Local>(s->id(), iterable->elementType(), nullptr, s->location());
        auto expr = std::make_shared<expression::Variable>(var, s->location());
        s->body()->scope()->insert(s->id(), expr);
    }
#endif    
}

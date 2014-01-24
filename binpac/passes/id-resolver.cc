
#include "id-resolver.h"

#include "../attribute.h"
#include "../declaration.h"
#include "../expression.h"
#include "../scope.h"
#include "../statement.h"
#include "../type.h"

using namespace binpac;
using namespace binpac::passes;

// A helper pass that replaces an ID expression with a different node within a subtree.
class IDExprReplacer : public ast::Pass<AstInfo>
{
public:
    IDExprReplacer(shared_ptr<ID> old, shared_ptr<Node> new_) : ast::Pass<AstInfo>("binpac::normalizer::IDExprReplacer", true) {
        _old = old;
        _new = new_;
    }

    virtual ~IDExprReplacer() {}

    bool run(shared_ptr<ast::NodeBase> node) override {
        return processAllPreOrder(node);
    }

protected:
    void visit(expression::ID* i) override {
        if ( i->id()->pathAsString() == _old->pathAsString() )
            i->replace(_new);
    }

private:
    shared_ptr<ID> _old;
    shared_ptr<Node> _new;
};

IDResolver::IDResolver() : Pass<AstInfo>("binpac::IDResolver", true)
{
}

IDResolver::~IDResolver()
{
}

bool IDResolver::run(shared_ptr<ast::NodeBase> module, bool report_unresolved)
{
    _report_unresolved = report_unresolved;
    return run(module);
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

    // std::cerr << "Lookup: " << i->id()->pathAsString() << std::endl;

    for ( auto i = nodes.rbegin(); i != nodes.rend(); i++ ) {
        auto n = *i;

        auto unit = ast::tryCast<type::Unit>(n);
        auto block = ast::tryCast<statement::Block>(n);
        auto item = ast::tryCast<type::unit::Item>(n);

        // std::cerr << "  block: " << block.get() << std::endl;
        // std::cerr << "  item: "  << item.get() << std::endl;
        // std::cerr << "  unit: "  << unit.get() << std::endl;

        if ( block ) {
            scope = block->scope();
            break;
        }

        if ( item ) {
            scope = item->scope();
            break;
        }

        if ( unit ) {
            scope = unit->scope();
            break;
        }
    }

    if ( ! scope ) {
        auto module = current<Module>();
        assert(module);
        scope = module->body()->scope();
        // std::cerr << "  module: " << module.get() << std::endl;
    }

    auto id = i->sharedPtr<expression::ID>();
    auto vals = scope->lookup(id->id(), true);

    if ( ! vals.size() ) {
        // We we're inside an externally defined hook, make sure to also
        // search the global scope of the module where it's defined.
        auto imod = i->firstParent<Module>();
        if ( imod ) {
            // std::cerr << "  imod: " << imod.get() << std::endl;
            // std::cerr << "  imod SCOPE: ";
            scope = imod->body()->scope();
            // scope->dump(std::cerr);
            vals = scope->lookup(id->id(), true);
        }
    }

    if ( ! vals.size() ) {
        if ( _report_unresolved )
            error(i, util::fmt("unknown ID %s", id->id()->pathAsString().c_str()));

        return;
    }

    if ( vals.size() > 1 ) {
        // Only functions can be overloaded.
        for ( auto v: vals ) {
            if ( ! ast::tryCast<expression::Function>(v) ) {
                error(i, util::fmt("ID %s defined more than once", id->id()->pathAsString().c_str()));
                return;
            }
        }

        // We resolve these later with overload-resolver.
        return;
    }

#if 0
    // Check use $$ here because we're going to replace it and thus the
    // valdiator won't see ID anymore.
    if ( id->id()->name() == "$$" ) {
        auto h = i->firstParent<Hook>();

        bool ok = false;

        if ( h && h->foreach() )
            ok = true;

        if ( (! h) && i->firstParent<type::Unit>() )
            ok = true;

        if ( ! ok )
            error(i, util::fmt("$$ not defined here"));
    }
#endif

    auto val = vals.front();

    id->replace(val);

    if ( id->id()->isScoped() )
        val->setScope(id->id()->scope());

}

void IDResolver::visit(expression::ListComprehension* c)
{
    auto iterable = ast::type::trait::asTrait<type::trait::Iterable>(c->input()->type().get());
    auto var = std::make_shared<variable::Local>(c->variable(), iterable->elementType());
    auto evar = std::make_shared<expression::Variable>(var);
    IDExprReplacer replacer(c->variable(), evar);

    if ( c->predicate() )
        replacer.run(c->predicate());

    replacer.run(c->output());
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

    auto vals = body->scope()->lookup(id, true);

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


    auto nt = ast::tryCast<expression::Type>(val);

    if ( ! nt ) {
        error(t, util::fmt("ID %s does not reference a type", id->pathAsString().c_str()));
        return;
    }

    auto tv = nt->typeValue();
    t->replace(tv);

    if ( ! tv->id() || ! (tv->id()->isScoped() && id->isScoped()) ) {
        tv->setID(id);

        if ( ! id->isScoped() )
            tv->setScope(current<Module>()->id()->name());
    }
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

    auto uid = std::make_shared<ID>(path);
    auto exprs = module->body()->scope()->lookup(uid, true);

    if ( ! exprs.size() ) {
        if ( _report_unresolved )
            error(h, "unknown id");
        return;
    }

    if ( exprs.size() > 1 ) {
        error(h, util::fmt("ID %s references more than one object", h->id()->pathAsString()));
        return;
    }

    auto expr = exprs.front();

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

    auto mid = h->firstParent<Module>()->id();
    assert(mid);

    unit->setID(std::make_shared<ID>(uid->pathAsString(mid)));
    unit->setScope(mid->name());
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
        internalError(f, "field::Unknown without scope ID");

    auto exprs = body->scope()->lookup(id, true);

    if ( ! exprs.size() ) {
        if ( _report_unresolved )
            error(f, util::fmt("unknown field ID %s", id->pathAsString()));
        return;
    }

    if ( exprs.size() > 1 ) {
        error(f, util::fmt("ID %s references more than one object", id->pathAsString()));
        return;
    }

    auto expr = exprs.front();

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

    nfield->scope()->setParent(f->scope()->parent());

    assert(nfield);
    f->replace(nfield);

    if ( f->anonymous() )
        nfield->setAnonymous();
}


#include "../declaration.h"
#include "../expression.h"
#include "../function.h"
#include "../scope.h"
#include "../statement.h"
#include "../variable.h"

#include "scope-builder.h"
#include "../context.h"

namespace binpac {

class ScopeClearer : public ast::Pass<AstInfo>
{
public:
    ScopeClearer() : Pass<AstInfo>("binpac::ScopeClearer", false) {}
    virtual ~ScopeClearer() {}

    bool run(shared_ptr<ast::NodeBase> module) override { return processAllPreOrder(module); }

protected:
    void visit(statement::Block* b) override { b->scope()->clear(); }

};

}

using namespace binpac;
using namespace binpac::passes;

ScopeBuilder::ScopeBuilder(CompilerContext* context) : Pass<AstInfo>("binpac::ScopeBuilder", false)
{
    _context = context;
}

ScopeBuilder::~ScopeBuilder()
{
}

bool ScopeBuilder::run(shared_ptr<ast::NodeBase> module)
{
    _module = ast::checkedCast<Module>(module);

    ScopeClearer clearer;
    clearer.run(module);

    if ( ! processAllPreOrder(module) )
        return false;

    auto m = ast::checkedCast<Module>(module);

    for ( auto i : m->importedIDs() ) {

        auto m1 = util::strtolower(i->pathAsString());
        auto m2 = util::strtolower(m->id()->pathAsString());

        if ( m1 == m2 )
            continue;

        shared_ptr<Module> other = _context->load(i->name());
        if ( ! other )
            fatalError("import failed");

        m->body()->scope()->addChild(other->id(), other->body()->scope());
    }

    return errors() == 0;
}

// When the scope builder runs, the validator may not yet have been executed.
// So check the stuff we need.
shared_ptr<Scope> ScopeBuilder::_checkDecl(Declaration* decl)
{
    auto id = decl->id();
    auto is_hook = dynamic_cast<declaration::Hook*>(decl);

    if ( ! id ) {
        error(decl, "declaration without an ID");
        return 0;
    }

    if ( id->isScoped() && ! is_hook && decl->linkage() != Declaration::IMPORTED ) {
        error(decl, "declared ID cannot have a scope");
        return 0;
    }

    auto block = current<statement::Block>();

    if ( ! block ) {
        error(decl, util::fmt("declaration of %s is not part of a block", decl->id()->name().c_str()));
        return 0;
    }

    return block->scope();
}

void ScopeBuilder::visit(declaration::Variable* v)
{
    auto scope = _checkDecl(v);

    if ( ! scope )
        return;

    auto var = v->variable()->sharedPtr<Variable>();
    auto expr = std::make_shared<expression::Variable>(var, var->location());
    scope->insert(v->id(), expr, true);
}

void ScopeBuilder::visit(declaration::Type* t)
{
    auto scope = _checkDecl(t);

    if ( ! scope )
        return;

    auto type = t->type();
    auto expr = shared_ptr<expression::Type>(new expression::Type(type, type->location()));
    scope->insert(t->id(), expr, true);

    // Link in any type-specific scope the type may define.
    auto tscope = t->type()->typeScope();

    if ( tscope ) {
        tscope->setParent(scope);
        scope->addChild(t->id(), tscope);
    }

    t->type()->setID(t->id());
    t->type()->setScope(_module->id()->name());
}

void ScopeBuilder::visit(declaration::Constant* c)
{
    auto scope = _checkDecl(c);

    if ( ! scope )
        return;

    scope->insert(c->id(), c->constant(), true);
}

void ScopeBuilder::visit(declaration::Function* f)
{
    auto scope = _checkDecl(f);

    if ( ! scope )
        return;

    auto func = f->function()->sharedPtr<Function>();
    auto expr = shared_ptr<expression::Function>(new expression::Function(func, func->location()));

    auto is_hook = dynamic_cast<declaration::Hook*>(f);

    if ( ! is_hook )
        scope->insert(f->id(), expr, true);

    if ( ! func->body() )
        // Just a declaration without implementation.
        return;

    // Add parameters to body's scope.
    scope = ast::checkedCast<statement::Block>(func->body())->scope();

    for ( auto p : func->type()->parameters() ) {
        auto pexpr = shared_ptr<expression::Parameter>(new expression::Parameter(p, p->location()));
        scope->insert(p->id(), pexpr, true);
    }
}

void ScopeBuilder::visit(declaration::Hook* h)
{
    auto hook = h->hook();

    // Add parameters to body's scope.
    auto scope = ast::checkedCast<statement::Block>(hook->body())->scope();

    for ( auto p : hook->parameters() ) {
        auto pexpr = shared_ptr<expression::Parameter>(new expression::Parameter(p, p->location()));
        scope->insert(p->id(), pexpr, true);
    }
}

void ScopeBuilder::visit(type::unit::item::GlobalHook* g)
{
    for ( auto hook : g->hooks() ) {
        // Add parameters to body's scope.
        auto scope = ast::checkedCast<statement::Block>(hook->body())->scope();

        for ( auto p : hook->parameters() ) {
            auto pexpr = shared_ptr<expression::Parameter>(new expression::Parameter(p, p->location()));
            scope->insert(p->id(), pexpr, true);
        }
    }
}


#include "hilti-intern.h"

using namespace hilti::passes;

shared_ptr<Scope> ScopeBuilder::_checkDecl(Declaration* decl)
{
    auto id = decl->id();
    auto is_hook = dynamic_cast<declaration::Hook*>(decl);

    if ( ! id ) {
        error(decl, "declaration without an ID");
        return 0;
    }

    if ( id->isScoped() && ! is_hook ) {
        error(decl, "declared ID cannot have a scope");
        return 0;
    }

    auto block = current<statement::Block>();

    if ( ! block ) {
        error(decl, util::fmt("declaration of %s is not part of a block", decl->id()->name().c_str()));
        return 0;
    }

    auto scope = block->scope();

    if ( (! is_hook) && scope->has(decl->id(), false) ) {
        error(decl, util::fmt("ID %s already declared", decl->id()->name().c_str()));
        return 0;
    }

    return scope;
}

bool ScopeBuilder::run(shared_ptr<Node> module)
{
    auto m = ast::as<Module>(module);
    m->body()->scope()->clear();

    if ( !  processAllPreOrder(module) )
        return true;

    for ( auto i : m->importedIDs() ) {
        shared_ptr<Module> other = hilti::loadModule(i, _libdirs, true);
        if ( ! other )
            fatalError("import failed");

        m->body()->scope()->addChild(other->id(), other->body()->scope());
    }

    return true;
}

void ScopeBuilder::visit(Module* m)
{
}

void ScopeBuilder::visit(statement::Block* b)
{
    if ( ! b->id() )
        return;

    auto func = current<hilti::Function>();

    if ( ! func ) {
        error(b, util::fmt("declaration of block is not part of a function"));
        return;
    }

    if ( ! func->body() )
        // Just a declaration without implementation.
        return;

    auto scope = func->body()->scope();

    if ( scope->has(b->id(), false) ) {
        error(b, util::fmt("ID %s already declared", b->id()->name().c_str()));
        return;
    }

    auto block = b->sharedPtr<statement::Block>();
    auto expr = shared_ptr<expression::Block>(new expression::Block(block, block->location()));
    scope->insert(b->id(), expr);
}

void ScopeBuilder::visit(declaration::Variable* v)
{
    auto scope = _checkDecl(v);

    if ( ! scope )
        return;

    auto var = v->variable()->sharedPtr<Variable>();
    auto expr = std::make_shared<expression::Variable>(var, var->location());
    scope->insert(v->id(), expr);
}

void ScopeBuilder::visit(declaration::Type* t)
{
    auto scope = _checkDecl(t);

    if ( ! scope )
        return;

    auto type = t->type();
    auto expr = shared_ptr<expression::Type>(new expression::Type(type, type->location()));
    scope->insert(t->id(), expr);

    // Link in any type-specific scope the type may define.
    auto tscope = t->type()->typeScope();

    if ( tscope ) {
        tscope->setParent(scope);
        scope->addChild(t->id(), tscope);
    }
}

void ScopeBuilder::visit(declaration::Constant* c)
{
    auto scope = _checkDecl(c);

    if ( ! scope )
        return;

    auto constant = c->constant();
    auto expr = shared_ptr<expression::Constant>(new expression::Constant(constant, constant->location()));
    scope->insert(c->id(), expr);
}

void ScopeBuilder::visit(declaration::Function* f)
{
    auto scope = _checkDecl(f);

    if ( ! scope )
        return;

    auto func = f->function()->sharedPtr<Function>();
    auto expr = shared_ptr<expression::Function>(new expression::Function(func, func->location()));

    if ( ! f->id()->isScoped() )
        scope->insert(f->id(), expr);

    if ( ! func->body() )
        // Just a declaration without implementation.
        return;

    // Add parameters to body's scope.
    scope = func->body()->scope();

    for ( auto p : func->type()->parameters() ) {
        auto pexpr = shared_ptr<expression::Parameter>(new expression::Parameter(p, p->location()));
        scope->insert(p->id(), pexpr);
    }
}

void ScopePrinter::visit(statement::Block* b)
{
    b->scope()->dump(std::cerr);
    std::cerr << "+++++" << std::endl;
}


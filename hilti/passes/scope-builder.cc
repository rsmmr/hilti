
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

    if ( ! processAllPreOrder(module) )
        return false;

    for ( auto i : m->importedIDs() ) {
        auto mscope = _context->scopeAlias(i);
        assert(mscope);
        assert(mscope->id());
        mscope->setParent(m->body()->scope());
        m->body()->scope()->addChild(mscope->id(), mscope);
    };

    return errors() == 0;
}

void ScopeBuilder::visit(Module* m)
{
}

void ScopeBuilder::visit(statement::Block* b)
{
    if ( ! b->id() )
        return;

    auto func = current<hilti::Function>();

    if ( ! func ) // TODO: Too bad current() doesn't follow the class hierarchy ... 
        func = current<hilti::Hook>();

    if ( ! func ) {
        error(b, util::fmt("declaration of block is not part of a function %p", b));
        return;
    }

    if ( ! func->body() )
        // Just a declaration without implementation.
        return;

    auto scope = ast::checkedCast<statement::Block>(func->body())->scope();

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

    scope->insert(c->id(), c->constant());
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
    scope = ast::checkedCast<statement::Block>(func->body())->scope();

    for ( auto p : func->type()->parameters() ) {
        auto pexpr = shared_ptr<expression::Parameter>(new expression::Parameter(p, p->location()));
        scope->insert(p->id(), pexpr);
    }
}

void ScopePrinter::visit(statement::Block* b)
{
    auto mod = current<Module>();

    _out << "Module " << ( mod ? mod->id()->name() : string("<null>") ) << std::endl;

    b->scope()->dump(_out);
    _out << std::endl;
    _out << "+++++" << std::endl;
    _out << std::endl;
}


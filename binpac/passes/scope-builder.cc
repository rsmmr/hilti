
#include "declaration.h"
#include "expression.h"
#include "function.h"
#include "scope.h"
#include "statement.h"
#include "variable.h"

#include "scope-builder.h"
#include "context.h"

using namespace binpac;
using namespace binpac::passes;

ScopeBuilder::ScopeBuilder(CompilerContext* context) : Pass<AstInfo>("ScopeBuilder")
{
    _context = context;
}

ScopeBuilder::~ScopeBuilder()
{
}

bool ScopeBuilder::run(shared_ptr<ast::NodeBase> module)
{
    auto m = ast::checkedCast<Module>(module);
    m->body()->scope()->clear();

    if ( ! processAllPreOrder(module) )
        return false;

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

    t->type()->setID(t->id());

    // If this is a unit, populate it's scope.
    auto unit = ast::tryCast<type::Unit>(t->type());

    if ( unit ) {
        auto uscope = unit->scope();
        uscope->insert(std::make_shared<ID>("self"), std::make_shared<expression::ParserState>(expression::ParserState::SELF, nullptr, unit, unit));
        uscope->setParent(current<Module>()->body()->scope());

        for ( auto p : unit->parameters() )
            uscope->insert(p->id(), std::make_shared<expression::ParserState>(expression::ParserState::PARAMETER, p->id(), unit));

        for ( auto i : unit->items() ) {

            auto iscope = i->scope();

            iscope->setParent(uscope);
            uscope->addChild(std::make_shared<ID>(util::fmt("__item_%s", i->id()->name())), iscope);
            auto dd = i->type();

            iscope->insert(std::make_shared<ID>("$$"),
                           std::make_shared<expression::ParserState>(expression::ParserState::DOLLARDOLLAR, nullptr, unit, dd));

            for ( auto h : i->hooks() ) {
                h->setUnit(unit);

                if ( h->body() ) {
                    h->body()->scope()->setParent(iscope);
                    iscope->addChild(std::make_shared<ID>(util::fmt("__hook_%p", h.get())), iscope);
                }

                if ( h->foreach() ) {
                    // The overrides the item's scope's $$.
                    auto dd = ast::type::checkedTrait<type::trait::Container>(i->type())->elementType();
                    h->body()->scope()->insert(std::make_shared<ID>("$$"),
                                               std::make_shared<expression::ParserState>(expression::ParserState::DOLLARDOLLAR, nullptr, unit, dd));
                }
            }
        }
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

    auto is_hook = dynamic_cast<declaration::Hook*>(f);

    if ( ! is_hook )
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


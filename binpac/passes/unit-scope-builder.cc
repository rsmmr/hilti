
#include "declaration.h"
#include "expression.h"
#include "scope.h"
#include "statement.h"

#include "unit-scope-builder.h"

using namespace binpac;
using namespace binpac::passes;

UnitScopeBuilder::UnitScopeBuilder() : Pass<AstInfo>("UnitScopeBuilder")
{
}

UnitScopeBuilder::~UnitScopeBuilder()
{
}

bool UnitScopeBuilder::run(shared_ptr<ast::NodeBase> module)
{
    return processAllPreOrder(module);
}

void UnitScopeBuilder::visit(declaration::Type* t)
{
    // If this is a unit, populate it's scope.
    auto unit = ast::tryCast<type::Unit>(t->type());

    if ( ! unit )
        return;

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
                assert(dd);
                h->body()->scope()->insert(std::make_shared<ID>("$$"),
                                           std::make_shared<expression::ParserState>(expression::ParserState::DOLLARDOLLAR, nullptr, unit, dd));
            }
        }
    }
}

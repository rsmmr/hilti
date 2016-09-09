
#include <util/util.h>

#include "../attribute.h"
#include "../declaration.h"
#include "../expression.h"
#include "../statement.h"
#include "../type.h"
#include "normalizer.h"

#include <binpac/autogen/operators/enum.h>
#include <binpac/autogen/operators/unit.h>

using namespace binpac;
using namespace binpac::passes;

Normalizer::Normalizer() : ast::Pass<AstInfo>("binpac::Normalizer", true)
{
}

Normalizer::~Normalizer()
{
}

bool Normalizer::run(shared_ptr<ast::NodeBase> ast)
{
    return processAllPostOrder(ast);
}

void Normalizer::visit(declaration::Type* t)
{
    auto unit = ast::rtti::tryCast<type::Unit>(t->type());

    if ( unit ) {
        if ( t->linkage() == Declaration::EXPORTED )
            unit->setExported();

        // Find fields sharing a name.
        std::map<std::string, int> fields;
        for ( auto f : unit->flattenedFields() ) {
            auto n = f->id()->name();
            fields[n] += 1;
        }

        for ( auto f : unit->flattenedFields() ) {
            auto n = f->id()->name();
            if ( fields[n] > 1 )
                f->setAliased();
        }
    }
}

void Normalizer::visit(declaration::Variable* t)
{
    auto var = t->variable();

    // The init expression could have had a type that wasn't resolvable
    // initially, potentially leaving the variable with an unknown type. At
    // this point that should be fixed.
    if ( var->init() && ast::rtti::isA<type::Unknown>(var->type()) )
        var->setType(var->init()->type());
}

void Normalizer::visit(type::unit::Item* i)
{
}

void Normalizer::visit(type::unit::item::Field* f)
{
    if ( ! f->type() )
        return;

    // Insert defaults for attributes lacking it.
    auto attributes = f->attributes();
    auto parseable = ast::type::checkedTrait<type::trait::Parseable>(f->type());

    std::set<string> seen;

    for ( auto attr : attributes->attributes() ) {
        for ( auto pattr : parseable->parseAttributes() ) {
            if ( pattr.key != attr->key() )
                continue;

            if ( attr->value() )
                continue;

            if ( pattr.default_ )
                // Insert default value.
                attr->setValue(pattr.default_);
        }

        seen.insert(attr->key());
    }

    for ( auto pattr : parseable->parseAttributes() ) {
        if ( ! pattr.implicit_ )
            continue;

        if ( seen.find(pattr.key) == seen.end() )
            // Insert attribute with default value.
            attributes->add(
                std::make_shared<Attribute>(pattr.key, pattr.default_, true, f->location()));
    }

    // Set type's bit order for bitmask fields.
    auto btype = ast::rtti::tryCast<type::Bitfield>(f->type());

    if ( btype ) {
        auto border = f->attributes()->lookup("bitorder");

        if ( border )
            btype->setBitOrder(border->value());
    }

    // If the field has a &parse, we ignore it for composing.
    if ( f->attributes()->has("parse") )
        f->setKind(type::unit::item::Field::PARSE);

    // If the field has a &convert attribute with an expression of the form
    // enum($$), add a &convert_back automatically for composing.
    if ( auto convert = attributes->lookup("convert") ) {
        if ( ! attributes->has("convert_back") ) {
            if ( auto call =
                     ast::rtti::tryCast<expression::operator_::enum_::Call>(convert->value()) ) {
                auto o = ast::rtti::checkedCast<expression::ResolvedOperator>(convert->value());
                auto c = ast::rtti::checkedCast<expression::Constant>(o->op2());
                auto elems = ast::rtti::checkedCast<constant::Tuple>(c->constant())->value();
                auto pstate = ast::rtti::tryCast<expression::ParserState>(elems.front());
                if ( pstate && pstate->kind() == expression::ParserState::DOLLARDOLLAR ) {
                    // Yes, found it.
                    expression_list ops = {};

                    auto npstate =
                        std::make_shared<expression::ParserState>(expression::ParserState::SELF,
                                                                  nullptr, pstate->unit(),
                                                                  pstate->unit());
                    auto mattr = std::make_shared<expression::MemberAttribute>(f->id());

                    ops = {npstate, mattr};
                    auto nattr = std::make_shared<
                        binpac::expression::UnresolvedOperator>(operator_::Attribute, ops);

                    auto ctype = std::make_shared<expression::Type>(f->type());
                    ops = {nattr, ctype};
                    auto ne =
                        std::make_shared<binpac::expression::UnresolvedOperator>(operator_::Cast,
                                                                                 ops);
                    auto convert_back = std::make_shared<Attribute>("convert_back", ne);

                    attributes->add(convert_back);
                }
            }
        }
    }
}

void Normalizer::visit(type::unit::item::field::Container* c)
{
}

void Normalizer::visit(binpac::expression::operator_::unit::SetPosition* i)
{
    auto unit = ast::rtti::checkedCast<type::Unit>(i->op1()->type());
    unit->enableBuffering();
}

void Normalizer::visit(binpac::expression::operator_::unit::Offset* i)
{
    auto unit = ast::rtti::checkedCast<type::Unit>(i->op1()->type());
    unit->enableBuffering();
}

void Normalizer::visit(binpac::expression::operator_::unit::Input* i)
{
    auto unit = ast::rtti::checkedCast<type::Unit>(i->op1()->type());
    unit->enableBuffering();
}

void Normalizer::visit(statement::Return* r)
{
    // If the function's return type is still unknown, derive it from the
    // expression's type.
    auto func = current<Function>();

    if ( ! func )
        return;

    auto rtype = func->type()->result()->type();

    if ( ast::rtti::isA<type::Unknown>(rtype) )
        rtype->replace(r->expression()->type());
}

void Normalizer::visit(binpac::expression::operator_::unit::TryAttribute* i)
{
    i->setUsesTryAttribute(true);
}

void Normalizer::visit(binpac::Expression* i)
{
    for ( auto n : i->childs(false) ) {
        if ( auto e = ast::rtti::tryCast<Expression>(n) ) {
            if ( e->usesTryAttribute() )
                i->setUsesTryAttribute(true);
        }
    }
}

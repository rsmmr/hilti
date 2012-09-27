
#include <util/util.h>

#include "normalizer.h"
#include "type.h"
#include "expression.h"
#include "attribute.h"
#include "statement.h"

using namespace binpac;
using namespace binpac::passes;

Normalizer::Normalizer() : ast::Pass<AstInfo>("Normalizer")
{
}

Normalizer::~Normalizer()
{
}

bool Normalizer::run(shared_ptr<ast::NodeBase> ast)
{
    return processAllPreOrder(ast);
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
            attributes->add(std::make_shared<Attribute>(pattr.key, pattr.default_, true, f->location()));
    }
}

void Normalizer::visit(type::unit::item::field::Container* c)
{
}

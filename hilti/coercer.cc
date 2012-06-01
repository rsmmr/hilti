
#include "id.h"
#include "expression.h"
#include "statement.h"
#include "module.h"
#include "coercer.h"

using namespace hilti;

void Coercer::visit(type::Integer* i)
{
    setResult(false);

    auto dst_b = ast::as<type::Bool>(arg1());

    if ( dst_b ) {
        setResult(true);
        return;
    }
}

void Coercer::visit(type::Reference* r)
{
    setResult(false);

    auto dst_ref = ast::as<type::Reference>(arg1());

    if ( dst_ref ) {
        setResult(dst_ref->wildcard());
        return;
    }

    auto dst_b = ast::as<type::Bool>(arg1());

    if ( dst_b ) {
        setResult(true);
        return;
    }
}

void Coercer::visit(type::Iterator* i)
{
    setResult(false);

    auto dst_iter = ast::as<type::Iterator>(arg1());

    if ( dst_iter ) {
        setResult(dst_iter->wildcard());
        return;
    }
}

void Coercer::visit(type::Tuple* t)
{
    setResult(false);

    auto dst = ast::as<type::Tuple>(arg1());

    if ( ! dst )
        return;

    if ( dst->wildcard() ) {
        setResult(true);
        return;
    }

    auto dst_types = dst->typeList();

    if ( dst_types.size() != t->typeList().size() )
        return;

    auto d = dst_types.begin();

    for ( auto e : t->typeList() ) {
        if ( ! canCoerceTo(e, *d++) )
            return;
    }

    setResult(true);
}

void Coercer::visit(type::Address* t)
{
    setResult(false);

    auto dst_net = ast::as<type::Network>(arg1());

    if ( dst_net ) {
        setResult(true);
        return;
    }
}

void Coercer::visit(type::Unset* t)
{
    setResult(true);
}

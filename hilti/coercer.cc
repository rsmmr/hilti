
#include "coercer.h"
#include "expression.h"
#include "id.h"
#include "module.h"
#include "statement.h"

using namespace hilti;

void Coercer::visit(type::Integer* i)
{
    setResult(false);

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        setResult(true);
        return;
    }

    // We don't allow integer coercion to larger widths here because we
    // wouldn't know if they are signed or not.
}

void Coercer::visit(type::Reference* r)
{
    setResult(false);

    if ( auto dst = ast::rtti::tryCast<type::Reference>(arg1()) ) {
        if ( ast::rtti::isA<type::RegExp>(r->argType()) &&
             ast::rtti::isA<type::RegExp>(dst->argType()) ) {
            setResult(true);
            return;
        }

        setResult(dst->wildcard());
        return;
    }

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        setResult(true);
        return;
    }
}

void Coercer::visit(type::Iterator* i)
{
    setResult(false);

    if ( auto dst = ast::rtti::tryCast<type::Iterator>(arg1()) ) {
        setResult(dst->wildcard());
        return;
    }
}

void Coercer::visit(type::Tuple* t)
{
    setResult(false);

    if ( auto rtype = ast::rtti::tryCast<type::Reference>(arg1()) ) {
        auto stype = ast::rtti::tryCast<type::Struct>(rtype->argType());

        if ( ! stype )
            return;

        // Coerce to struct is fine if (1) it's an emptuple, or (2) all
        // element coerce.

        if ( t->typeList().size() == 0 ) {
            setResult(true);
            return;
        }

        if ( t->typeList().size() != stype->typeList().size() )
            return;

        for ( auto i : util::zip2(t->typeList(), stype->typeList()) ) {
            if ( ast::rtti::isA<type::Unset>(i.first) )
                continue;

            if ( ! canCoerceTo(i.first, i.second) )
                return;
        }

        setResult(true);
        return;
    }

    if ( auto dst = ast::rtti::tryCast<type::Tuple>(arg1()) ) {
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
}

void Coercer::visit(type::Address* t)
{
    setResult(false);

    if ( ast::rtti::isA<type::Network>(arg1()) ) {
        setResult(true);
        return;
    }
}

void Coercer::visit(type::CAddr* c)
{
    setResult(false);

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        setResult(true);
        return;
    }
}

void Coercer::visit(type::Unset* c)
{
    // Generic unsets coerce into any type as the corresponding default
    // value.
    setResult(true);
}

void Coercer::visit(type::Union* c)
{
    setResult(false);

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        setResult(true);
        return;
    }
}

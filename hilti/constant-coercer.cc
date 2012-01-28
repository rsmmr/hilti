
#include "id.h"
#include "expression.h"
#include "statement.h"
#include "module.h"
#include "constant-coercer.h"

using namespace hilti;

void ConstantCoercer::visit(constant::Integer* i)
{
    setResult(nullptr);

    auto dst_i = ast::as<type::Integer>(arg1());

    if ( dst_i && i->value() <=  (2l << (dst_i->width() - 1) - 1)
               && i->value() >= -(2l << (dst_i->width() - 1)) ) {
        auto c = new constant::Integer(i->value(), dst_i->width(), i->location());
        setResult(shared_ptr<Constant>(c));
        return;
    }

    auto dst_b = ast::as<type::Bool>(arg1());

    if ( dst_b ) {
        auto c = new constant::Bool(i->value() != 0, i->location());
        setResult(shared_ptr<Constant>(c));
    }
}

void ConstantCoercer::visit(constant::Tuple* t)
{
    setResult(nullptr);

    auto dst = ast::as<type::Tuple>(arg1());

    if ( ! dst )
        return;

    if ( dst->wildcard() ) {
        setResult(t->sharedPtr<constant::Tuple>());
        return;
    }

    auto dst_types = dst->typeList();

    if ( dst_types.size() != t->value().size() )
        return;

    auto d = dst_types.begin();
    constant::Tuple::element_list coerced_elems;

    for ( auto e : t->value() ) {
        auto coerced = e->coerceTo(*d++);
        if ( ! coerced )
            return;

        coerced_elems.push_back(coerced);
    }

    setResult(shared_ptr<Constant>(new constant::Tuple(coerced_elems, t->location())));
}

void ConstantCoercer::visit(constant::Reference* r)
{
    setResult(nullptr);

    auto dst_ref = ast::as<type::Reference>(arg1());

    if ( dst_ref->wildcard() ) {
        // Null coerces into all reference types.
        setResult(r->sharedPtr<constant::Tuple>());
        return;
    }

    if ( dst_ref ) {
        // Null coerces into all reference types.
        setResult(r->sharedPtr<constant::Tuple>());
        return;
    }

    auto dst_bool = ast::as<type::Bool>(arg1());

    if ( dst_bool ) {
        // The only possible constant is null, and that's false.
        auto c = new constant::Bool(false, r->location());
        setResult(shared_ptr<Constant>(c));
    }
}

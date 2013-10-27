
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

    if ( dst_i && i->value() <=   1l << dst_i->width()
               && i->value() >= -(1l << dst_i->width()) ) {
        auto c = new constant::Integer(i->value(), dst_i->width(), i->location());
        setResult(shared_ptr<Constant>(c));
        return;
    }

    auto dst_b = ast::as<type::Bool>(arg1());

    if ( dst_b ) {
        auto c = new constant::Bool(i->value() != 0, i->location());
        setResult(shared_ptr<Constant>(c));
    }

    auto dst_d = ast::as<type::Double>(arg1());

    if ( dst_d ) {
        auto c = new constant::Double(i->value(), i->location());
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

    if ( dst_ref && dst_ref->wildcard() ) {
        // Null coerces into all reference types.
        setResult(r->sharedPtr<constant::Reference>());
        return;
    }

    if ( dst_ref ) {
        // Null coerces into all reference types.
        setResult(std::make_shared<constant::Reference>(dst_ref));
        return;
    }

    auto dst_bool = ast::as<type::Bool>(arg1());

    if ( dst_bool ) {
        // The only possible constant is null, and that's false.
        auto c = new constant::Bool(false, r->location());
        setResult(shared_ptr<Constant>(c));
    }

    auto dst_caddr = ast::as<type::CAddr>(arg1());

    if ( dst_caddr ) {
        // Null also coerces into a null pointer.
        auto c = new constant::CAddr(r->location());
        setResult(shared_ptr<Constant>(c));
    }
}

void ConstantCoercer::visit(constant::Address* i)
{
    setResult(nullptr);

    auto dst_net = ast::as<type::Network>(arg1());

    if ( dst_net ) {
        auto w = (i->value().family == constant::AddressVal::IPv4 ? 32 : 128);
        auto c = new constant::Network(i->value(), w, i->location());
        setResult(shared_ptr<Constant>(c));
        return;
    }
}

void ConstantCoercer::visit(constant::CAddr* t)
{
    setResult(nullptr);

    auto dst_bool = ast::as<type::Bool>(arg1());

    if ( dst_bool ) {
        // The only possible constant is null, and that's false.
        auto c = new constant::Bool(false, t->location());
        setResult(shared_ptr<Constant>(c));
    }
}


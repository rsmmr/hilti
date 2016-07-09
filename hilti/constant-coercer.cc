
#include "constant-coercer.h"
#include "expression.h"
#include "id.h"
#include "module.h"
#include "statement.h"

using namespace hilti;

void ConstantCoercer::visit(constant::Integer* i)
{
    setResult(nullptr);

    if ( auto dst = ast::rtti::tryCast<type::Integer>(arg1()) ) {
        if ( i->value() <= 1l << dst->width() && i->value() >= -(1l << dst->width()) ) {
            auto c = new constant::Integer(i->value(), dst->width(), i->location());
            setResult(shared_ptr<Constant>(c));
            return;
        }
    }

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        auto c = new constant::Bool(i->value() != 0, i->location());
        setResult(shared_ptr<Constant>(c));
    }

    if ( ast::rtti::isA<type::Double>(arg1()) ) {
        auto c = new constant::Double(i->value(), i->location());
        setResult(shared_ptr<Constant>(c));
    }
}

void ConstantCoercer::visit(constant::Tuple* t)
{
    setResult(nullptr);

    auto dst = ast::rtti::tryCast<type::Tuple>(arg1());

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

    // The only possible constant is null.

    if ( auto dst = ast::rtti::tryCast<type::Reference>(arg1()) ) {
        // Null coerces into all reference types.
        if ( dst->wildcard() ) {
            // Null coerces into all reference types.
            setResult(r->sharedPtr<constant::Reference>());
            return;
        }

        setResult(std::make_shared<constant::Reference>(dst));
        return;
    }

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        // Null is false.
        auto c = new constant::Bool(false, r->location());
        setResult(shared_ptr<Constant>(c));
    }

    if ( ast::rtti::isA<type::CAddr>(arg1()) ) {
        // Null coerces into a null pointer.
        auto c = new constant::CAddr(r->location());
        setResult(shared_ptr<Constant>(c));
    }
}

void ConstantCoercer::visit(constant::Address* i)
{
    setResult(nullptr);

    if ( ast::rtti::isA<type::Network>(arg1()) ) {
        auto w = (i->value().family == constant::AddressVal::IPv4 ? 32 : 128);
        auto c = new constant::Network(i->value(), w, i->location());
        setResult(shared_ptr<Constant>(c));
        return;
    }
}

void ConstantCoercer::visit(constant::CAddr* t)
{
    setResult(nullptr);

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        // The only possible constant is null, and that's false.
        auto c = new constant::Bool(false, t->location());
        setResult(shared_ptr<Constant>(c));
    }
}

void ConstantCoercer::visit(constant::Unset* t)
{
    // Coerces into a typed unset, which will eventually codegen to the
    // type's default value.
    setResult(std::make_shared<constant::Unset>(arg1()));
}

void ConstantCoercer::visit(constant::Union* t)
{
    setResult(nullptr);

    if ( ast::rtti::isA<type::Bool>(arg1()) ) {
        bool has_expr = t->expression().get();
        auto c = new constant::Bool(has_expr, t->location());
        setResult(shared_ptr<Constant>(c));
    }

    if ( auto dst = ast::rtti::tryCast<type::Union>(arg1()) ) {
        auto c = new constant::Union(dst, t->id(), t->expression(), t->location());
        setResult(shared_ptr<Constant>(c));
    }
}

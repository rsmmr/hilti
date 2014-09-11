
#include "coercer.h"
#include "expression.h"

using namespace binpac;

bool Coercer::canCoerceTo(shared_ptr<Type> src, shared_ptr<Type> dst, string* error)
{
    return _coerceTo(nullptr, src, dst, nullptr, error);

}
shared_ptr<Expression> Coercer::coerceTo(shared_ptr<Expression> expr, shared_ptr<Type> dst)
{
    shared_ptr<Expression> result;
    if ( ! _coerceTo(expr, expr->type(), dst, &result, nullptr) )
        return nullptr;

    assert(result);
    return result;
}

bool Coercer::_coerceTo(shared_ptr<Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst, shared_ptr<Expression>* result, string* error)
{
#if 0
    std::cerr << "_coerceTo " << (expr ? expr->render() : string("(no expr)")) << ": " << src->render() << " -> " << dst->render() << std::endl;
#endif

    if ( ast::tryCast<type::Any>(dst) ) {
        if ( result )
            *result = expr;

        return true;
    }

    // Sometimes comparision isn't fully symmetrics, like with optional arguments.
    if ( src->equal(dst) || dst->equal(src) ) {
        // FIXME: For coercion, even anys needs to go through the code below.
        // However, standard equal() checking always flags them as equal.
        // Should we remove that from the standard implementation? But that
        // might break quite a bit, includign inside HILTI.
        if ( ! (src->matchesAny() || dst->matchesAny()) ) {
            if ( result )
                *result = expr;

            return true;
        }
    }

    if ( src->render() == dst->render() )
	{
	// FIXME: This a hack to get obvious matches to succeed. But we
	// should really implement equal() consistently instead.
	if ( result )
                *result = expr;

	return true;
	}

    auto t = ast::tryCast<type::OptionalArgument>(dst);

    if ( t )
        return _coerceTo(expr, src, t->argType(), result, error);

    if ( ! expr )
        expr = std::make_shared<expression::PlaceHolder>(src);

    expression_list ops = { expr, std::make_shared<expression::Type>(dst) };
    auto matches = OperatorRegistry::globalRegistry()->getMatching(operator_::Coerce, ops, false);

    if ( ! matches.size() ) {
        if ( error )
            *error = util::fmt("cannot coerce type %s to %s", src->render(), dst->render());

        return false;
    }

    if ( matches.size() > 1 ) {
        if ( error )
            *error = util::fmt("coerce from type %s to %s is ambigious", src->render(), dst->render());

        return false;
    }

    auto match = matches.front();
    auto op = match.first;
    ops = match.second;

    if ( result ) {
        auto module = expr->firstParent<Module>();
        *result = OperatorRegistry::globalRegistry()->resolveOperator(op, ops, module, expr->location());
    }

#if 0
    if ( result ) {
        if ( expr )
            std::cerr << "  => " << (*result)->render() << " / " << typeid(*(*result).get()).name() << std::endl;
    }
#endif

    return true;
}

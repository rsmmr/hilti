
#include "id.h"
#include "expression.h"
#include "statement.h"
#include "module.h"
#include "constant.h"

using namespace hilti;

static shared_ptr<Type> _tuple_type(const constant::Tuple::element_list& elems, const Location& l)
{
    type::Tuple::type_list types;
    for ( auto e : elems )
        types.push_back(e->type());

    return shared_ptr<Type>(new type::Tuple(types, l));
}

constant::Tuple::Tuple(const element_list& elems, const Location& l) : Constant(l)
{
    _elems = elems;

    for ( auto e : elems )
        addChild(e);
}

shared_ptr<Type> constant::Tuple::type() const
{
    type::Tuple::type_list types;

    for ( auto e : _elems )
        types.push_back(e->type());

    return shared_ptr<type::Tuple>(new type::Tuple(types, location()));
}

shared_ptr<Type> constant::Reference::type() const
{
    return shared_ptr<type::Reference>(new type::Reference(location()));
}

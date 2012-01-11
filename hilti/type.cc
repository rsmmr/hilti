
#include "expression.h"
#include "type.h"
#include "variable.h"
#include "function.h"

using namespace hilti;

type::trait::Trait::~Trait()
{
}

string type::trait::parameter::Type::repr() const
{
    return _type->repr();
}

type::Any::~Any() {}
type::Unknown::~Unknown() {}
type::Unset::~Unset() {}
type::Type::~Type() {}
type::Block::~Block() {}
type::Module::~Module() {}
type::Void::~Void() {}
type::String::~String() {}
type::Integer::~Integer() {}
type::Bool::~Bool() {}

string type::trait::Parameterized::repr() const
{
    if ( dynamic_cast<const hilti::Type*>(this)->wildcard() )
        return reprPrefix() + "<*>";

    string t = reprPrefix() + "<";

    bool first = true;

    for ( auto p : parameters() ) {
        if ( ! first )
            t += ",";

        t += p->repr();
        first = false;
    }

    return t + ">";
}

type::Function::Function(shared_ptr<hilti::function::Parameter> result, const function::parameter_list& args, hilti::type::function::CallingConvention cc, const Location& l)
    : hilti::Type(l), ast::type::mixin::Function<AstInfo>(this, result, args)
{
    _cc = cc;
}

type::function::Parameter::Parameter(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l)
    : ast::type::mixin::function::Parameter<AstInfo>(id, type, constant, default_value, l)
{
}

type::function::Parameter::Parameter(shared_ptr<Type> type, bool constant, Location l)
    : ast::type::mixin::function::Parameter<AstInfo>(type, constant, l)
{
}

type::Tuple::Tuple(const Location& l) : ValueType(l)
{
    setWildcard(true);
}

type::Tuple::Tuple(const type_list& types, const Location& l) : ValueType(l)
{
    _types = types;

    for ( auto t : types ) {
        auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Type(t));
        _parameters.push_back(p);
        addChild(p);
        addChild(t);
    }
}

const type::Tuple::type_list& type::Tuple::typeList() const
{
    return _types;
}



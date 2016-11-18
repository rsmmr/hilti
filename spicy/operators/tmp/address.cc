
#include "address.h"


#line 0 "/Users/robin/work/hilti/spicy/operators/address.cc"

#line 1 "/Users/robin/work/hilti/spicy/operators/address.cc"

AST_RTTI_BEGIN(spicy::operator_::address::Equal, spicy_operator_address_equal)
AST_RTTI_PARENT(spicy::Operator)
AST_RTTI_END(spicy_operator_address_equal)
AST_RTTI_CAST_BEGIN(spicy::operator_::address::Equal)
AST_RTTI_CAST_PARENT(spicy::Operator)
AST_RTTI_CAST_END()

namespace spicy {

operator_::address::Equal::Equal() : Operator(operator_::Equal, _factory)
{
}

operator_::address::Equal::~Equal()
{
}

expression::operator_::address::Equal::Equal(shared_ptr<Operator> op, const expression_list& ops,
                                             shared_ptr<spicy::Module> module, const Location& l)
    : expression::ResolvedOperator(op, ops, module, l)
{
}

#line 2 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Equal::__typeOp1() const
{
    return std::make_shared<type::Address>();
};
#line 3 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Equal::__typeOp2() const
{
    return std::make_shared<type::Address>();
};

#line 5 "/Users/robin/work/hilti/spicy/operators/address.cc"
string operator_::address::Equal::__doc() const
{
    return "Compares two address values, returning ``True`` if they are equal.";
};

#line 7 "/Users/robin/work/hilti/spicy/operators/address.cc"
void operator_::address::Equal::__validate()

{
}

#line 11 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Equal::__typeResult() const

{
    return std::make_shared<type::Bool>();
}
#line 15 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<expression::ResolvedOperator> operator_::address::Equal::_factory(
    shared_ptr<Operator> op, const expression_list& ops, shared_ptr<spicy::Module> module,
    const Location& l)
{
    return shared_ptr<expression::ResolvedOperator>(
        new expression::operator_::address::Equal(op, ops, module, l));
}

void __register_address_Equal()
{
    shared_ptr<Operator> op(new operator_::address::Equal);
    OperatorRegistry::globalRegistry()->addOperator(op);
}

} // namespace spicy

AST_RTTI_BEGIN(spicy::expression::resolved::address::Equal, spicy_expression_resolved_address_equal)
AST_RTTI_PARENT(spicy::operator ::ResolvedOperator)
AST_RTTI_END(spicy_expression_resolved_address_equal)
AST_RTTI_CAST_BEGIN(spicy::expression::resolved::address::Equal)
AST_RTTI_CAST_PARENT(spicy::expression::ResolvedOperator)
AST_RTTI_CAST_END()


#line 17 "/Users/robin/work/hilti/spicy/operators/address.cc"

AST_RTTI_BEGIN(spicy::operator_::address::Family, spicy_operator_address_family)
AST_RTTI_PARENT(spicy::Operator)
AST_RTTI_END(spicy_operator_address_family)
AST_RTTI_CAST_BEGIN(spicy::operator_::address::Family)
AST_RTTI_CAST_PARENT(spicy::Operator)
AST_RTTI_CAST_END()

namespace spicy {

operator_::address::Family::Family() : Operator(operator_::MethodCall, _factory)
{
}

operator_::address::Family::~Family()
{
}

expression::operator_::address::Family::Family(shared_ptr<Operator> op, const expression_list& ops,
                                               shared_ptr<spicy::Module> module, const Location& l)
    : expression::ResolvedOperator(op, ops, module, l)
{
}

#line 18 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Family::__typeOp1() const
{
    return std::make_shared<type::Address>();
};
#line 19 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Family::__typeOp2() const
{
    return std::make_shared<type::MemberAttribute>(std::make_shared<ID>("family"));
};

#line 21 "/Users/robin/work/hilti/spicy/operators/address.cc"
string operator_::address::Family::__doc() const
{
    return "Returns the IP family of an address value.";
};
#line 22 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Family::__docTypeResult() const
{
    return std::make_shared<type::TypeByName>("spicy::AddrFamily");
};

#line 24 "/Users/robin/work/hilti/spicy/operators/address.cc"
void operator_::address::Family::__validate()

{
}

#line 28 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<Type> operator_::address::Family::__typeResult() const

{
    return std::make_shared<type::TypeByName>(std::make_shared<ID>("Spicy::AddrFamily"));
}
#line 32 "/Users/robin/work/hilti/spicy/operators/address.cc"
shared_ptr<expression::ResolvedOperator> operator_::address::Family::_factory(
    shared_ptr<Operator> op, const expression_list& ops, shared_ptr<spicy::Module> module,
    const Location& l)
{
    return shared_ptr<expression::ResolvedOperator>(
        new expression::operator_::address::Family(op, ops, module, l));
}

void __register_address_Family()
{
    shared_ptr<Operator> op(new operator_::address::Family);
    OperatorRegistry::globalRegistry()->addOperator(op);
}

} // namespace spicy

AST_RTTI_BEGIN(spicy::expression::resolved::address::Family,
               spicy_expression_resolved_address_family)
AST_RTTI_PARENT(spicy::operator ::ResolvedOperator)
AST_RTTI_END(spicy_expression_resolved_address_family)
AST_RTTI_CAST_BEGIN(spicy::expression::resolved::address::Family)
AST_RTTI_CAST_PARENT(spicy::expression::ResolvedOperator)
AST_RTTI_CAST_END()

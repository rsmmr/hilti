
#include "operator.h"
#include "expression.h"
#include "coercer.h"

#include "passes/validator.h"

using namespace binpac;

OperatorRegistry* OperatorRegistry::_registry = 0;

Operator::Operator(operator_::Kind kind, expression_factory factory)
{
    _kind = kind;
    _factory = factory;
}

Operator::~Operator()
{
}

operator_::Kind Operator::kind()
{
    return _kind;
}

bool Operator::match(const expression_list& ops, bool coerce)
{
    std::list<shared_ptr<Type>> types = { __typeOp1(), __typeOp2(), __typeOp3() };

    auto o = ops.begin();
    auto t = types.begin();

    while ( o != ops.end() ) {
        if ( t == types.end() )
            // Too many arguments.
            return false;

        if ( coerce ) {
            if ( ! Coercer().canCoerceTo((*o)->type(), *t) )
                return false;
        }

        else {
            if ( ! (*o)->type()->equal(*t) )
                return false;
        }

        ++o;
        ++t;
    }

    // All remaining operands must be options.
    while ( t != types.end() ) {
        if ( *t && ! ast::isA<type::OptionalArgument>(*t) )
            return false;

        ++t;
    }

    // Match so far, now do any operand specific matching.
    __operands = ops;
    bool result = __match();
    __operands.clear();

    return result;
}

void Operator::validate(passes::Validator* vld, shared_ptr<Type> result, const expression_list& ops)
{
    _validator = vld;
    __operands = ops;
    __validate();
    __operands.clear();
    _validator = nullptr;

    auto expected = type(ops);

    if ( ! Coercer().canCoerceTo(expected, result) )
        error(result, util::fmt("incompatible operator result (expected %s, but have %s)",
                                expected->render().c_str(), result->render().c_str()));
}

shared_ptr<Type> Operator::type(const expression_list& ops)
{
    __operands = ops;
    auto result = __typeResult();
    __operands.clear();
    return result;
}

string Operator::render() const
{
    return "<no render() for operators yet>";
}

void Operator::error(Node* op, string msg) const
{
    assert(_validator);
    return _validator->error(op, msg);
}

void Operator::error(shared_ptr<Node> op, string msg) const
{
    assert(_validator);
    return _validator->error(op, msg);
}

void Operator::validate(passes::Validator* v, const expression_list& ops)
{
    _validator = v;
    __operands = ops;
    __validate();
    __operands.clear();
    _validator = nullptr;
}

OperatorRegistry::OperatorRegistry()
{
}

OperatorRegistry::~OperatorRegistry()
{
}

operator_list OperatorRegistry::getMatching(operator_::Kind kind, const expression_list& ops) const
{
    operator_list matches;

    // We first try to find direct matches without coercing types. If that
    // finds something, we're done. Otherwise, we try coercion.

    auto m = _operators.equal_range(kind);

    for ( auto n = m.first; n != m.second; ++n ) {
        auto op = n->second;

        if ( op->match(ops, false) )
            matches.push_back(op);
    }

    if ( matches.size() )
        return matches;

    for ( auto n = m.first; n != m.second; ++n ) {
        auto op = n->second;

        if ( op->match(ops, true) )
            matches.push_back(op);
    }

    return matches;
}

operator_list OperatorRegistry::byKind(operator_::Kind kind) const
{
    operator_list ops;

    auto m = _operators.equal_range(kind);

    for ( auto n = m.first; n != m.second; ++n )
        ops.push_back(n->second);

    return ops;
}

shared_ptr<expression::ResolvedOperator> OperatorRegistry::resolveOperator(shared_ptr<Operator> op, const expression_list& ops, const Location& l)
{
    return (op->factory())(op, ops, l);
}

void OperatorRegistry::addOperator(shared_ptr<Operator> op)
{
    _operators.insert(std::make_pair(op->kind(), op));
}

OperatorRegistry* OperatorRegistry::globalRegistry()
{
    if ( ! _registry )
        _registry = new OperatorRegistry();

    return _registry;
}

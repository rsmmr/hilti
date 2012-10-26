
#include <stdlib.h>
#include <cxxabi.h>

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

    int i = 1;

    while ( o != ops.end() ) {
        if ( t == types.end() )
            // Too many arguments.
            return false;

        if ( coerce ) {
            if ( ! Coercer().canCoerceTo((*o)->type(), *t) )
                return false;
        }

        else {
            assert((*o)->type());

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

bool Operator::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> type)
{
    if ( op->canCoerceTo(type) )
        return true;

    error(op, util::fmt("operand is of type %s but expected type", op->type(), type));
    return false;
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

bool Operator::checkCallArgs(shared_ptr<Expression> tuple, const type_list& types)
{
    auto const_ = ast::checkedCast<expression::Constant>(tuple);
    auto elems = ast::checkedCast<constant::Tuple>(const_->constant())->value();

    if ( elems.size() != types.size() ) {
        error(tuple, "wrong number of arguments");
        return false;
    }

    auto e = elems.begin();
    auto t = types.begin();

    while ( e != elems.end() ) {
        if ( ! (*e)->canCoerceTo(*t) ) {
            error(*e, "argument type mismatch");
            return false;
        }

        ++e;
        ++t;
    }

    return true;
}

string _clsName(shared_ptr<Type> t)
{
    if ( ! t )
        return "";

    int status;
    return abi::__cxa_demangle(typeid(*t).name(), 0, 0, &status);
}

Operator::Info Operator::info() const
{
    auto i = operator_::OperatorDefinitions.find(_kind);
    assert(i != operator_::OperatorDefinitions.end());

    Info info;
    info.kind = _kind;
    info.kind_txt = i->second.kind_txt;
    info.description = __doc();
    info.render = render();
    info.type_op1 = __typeOp1();
    info.type_op2 = __typeOp2();
    info.type_op3 = __typeOp3();
    info.class_op1 = _clsName(info.type_op1);
    info.class_op2 = _clsName(info.type_op2);
    info.class_op3 = _clsName(info.type_op3);
    return info;
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

const OperatorRegistry::operator_map& OperatorRegistry::getAll() const
{
    return _operators;
}

string Operator::render() const
{
    auto op1 = __typeOp1();
    auto op2 = __typeOp2();
    auto op3 = __typeOp3();

    auto i = operator_::OperatorDefinitions.find(_kind);
    assert(i != operator_::OperatorDefinitions.end());

    auto op = (*i).second;

    if ( op.type == operator_::UNARY_PREFIX )
        return util::fmt("%s%s", op.display, op1->render());

    if ( op.type == operator_::UNARY_POSTFIX )
        return util::fmt("%s%s", op1->render(), op.display);

    if ( op.type == operator_::BINARY )
        return util::fmt("%s %s %s", op1->render(), op.display, op2->render());

    switch ( op.kind ) {
     case operator_::Attribute:
        return util::fmt("%s.%s", op1->render(), op2->render());

     case operator_::AttributeAssign:
        return util::fmt("%s.%s = %s", op1->render(), op2->render(), op3->render());

     case operator_::Call:
        return util::fmt("%s(%s)", op1->render(), op2->render());

     case operator_::DecrPostfix:
        return util::fmt("%s--", op1->render());

     case operator_::HasAttribute:
        return util::fmt("%s?.%s", op1->render(), op2->render());

     case operator_::IncrPostfix:
        return util::fmt("%s++", op1->render());

     case operator_::Index:
        return util::fmt("%s[%s]", op1->render(), op2->render());

     case operator_::IndexAssign:
        return util::fmt("%s[%s] = %s", op1->render(), op2->render(), op3->render());

     case operator_::MethodCall:
        return util::fmt("%s.%s(%s)", op1->render(), op2->render(), op3->render());

     case operator_::Size:
        return util::fmt("|%s|", op1->render());

     default:
        fprintf(stderr, "unknown operator %d in Operator::render()", (int)op.kind);
        assert(false);
    }
}



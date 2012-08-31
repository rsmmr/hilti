
#include "expression.h"

#include "passes/printer.h"

using namespace binpac;

string Expression::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());

    string r = s.str();

    if ( scope().size() )
        r = r + " " + util::fmt("[scope: %s]", scope().c_str());

    return r;
}

Expression::operator string()
{
    return render();
}

Expression::Expression(const Location& l) : ast::Expression<AstInfo>(l)
{
}

bool Expression::initializer() const
{
    return false;
}

expression::CustomExpression::CustomExpression(const Location& l) : Expression(l)
{
}

shared_ptr<Type> expression::CustomExpression::type() const
{
    // Must be overridden.
    assert(false);
}

bool expression::CustomExpression::isConstant() const
{
    return false;
}

bool expression::CustomExpression::canCoerceTo(shared_ptr<Type> target) const
{
    return Coercer().canCoerceTo(type(), target);
}

shared_ptr<binpac::Expression> expression::CustomExpression::coerceTo(shared_ptr<Type> target)
{
#ifdef DEBUG
    if ( ! canCoerceTo(target) ) {
        std::cerr << util::fmt("cannot coerce expression of type %s to type %s",
                               render().c_str(),
                               target->render().c_str()) << std::endl;
    }
#endif

    auto t = type();

    if ( t->equal(target) )
        return sharedPtr<binpac::Expression>();

    auto coerced = new CoercedExpression(sharedPtr<binpac::Expression>(), target, location());
    return shared_ptr<typename AstInfo::expression>(coerced);
}

expression::List::List(const expression_list& exprs, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::List<AstInfo>(this, exprs)
{
}

expression::Constant::Constant(shared_ptr<binpac::Constant> constant, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Constant<AstInfo>(this, constant)
{
}

expression::Ctor::Ctor(shared_ptr<binpac::Ctor> ctor, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Ctor<AstInfo>(this, ctor)
{
}

expression::Module::Module(shared_ptr<binpac::Module> module, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Module<AstInfo>(this, module)
{
}

expression::Type::Type(shared_ptr<binpac::Type> type, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Type<AstInfo>(this, type)
{
}

expression::Variable::Variable(shared_ptr<binpac::Variable> var, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Variable<AstInfo>(this, var)
{
}

expression::ID::ID(shared_ptr<binpac::ID> id, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::ID<AstInfo>(this, id)
{
}

expression::Parameter::Parameter(shared_ptr<binpac::type::function::Parameter> param, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Parameter<AstInfo>(this, param)
{
}

expression::Function::Function(shared_ptr<binpac::Function> func, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Function<AstInfo>(this, func)
{
}

expression::Coerced::Coerced(shared_ptr<binpac::Expression> expr, shared_ptr<binpac::Type> dst, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::Coerced<AstInfo>(this, expr, dst)
{
}

expression::CodeGen::CodeGen(shared_ptr<binpac::Type> type, void* cookie, const Location& l)
    : binpac::Expression(l), ast::expression::mixin::CodeGen<AstInfo>(this, type, cookie)
{
}

expression::MemberAttribute::MemberAttribute(shared_ptr<binpac::ID> id, const Location& l)
    : CustomExpression(l)
{
    _attribute = id;
    _type = std::make_shared<type::MemberAttribute>(id, l);
    addChild(_attribute);
    addChild(_type);
}

shared_ptr<binpac::ID> expression::MemberAttribute::id() const
{
    return _attribute;
}

shared_ptr<Type> expression::MemberAttribute::type() const
{
    return _type;
}

expression::ParserState::ParserState(Kind kind, shared_ptr<binpac::ID> id, shared_ptr<Type> type, const Location& l) : CustomExpression(l)
{
    _kind = kind;
    _id = id;
    _type = _type ? std::make_shared<type::Unknown>(l) : type;

    addChild(_type);
    addChild(_id);
    addChild(_type);
}

expression::ParserState::Kind expression::ParserState::kind() const
{
    return _kind;
}

shared_ptr<binpac::ID> expression::ParserState::id() const
{
    return _id;
}

shared_ptr<Type> expression::ParserState::type() const
{
    return _type;
}

void expression::ParserState::setType(shared_ptr<binpac::Type> type)
{
    removeChild(_type);
    _type = type;
    addChild(_type);
}

expression::UnresolvedOperator::UnresolvedOperator(binpac::operator_::Kind kind, const expression_list& ops, const Location& l)
    : CustomExpression(l)
{
    _kind = kind;

    for ( auto op : ops )
        _ops.push_back(op);

    for ( auto op : _ops )
        addChild(op);
}

expression::UnresolvedOperator::~UnresolvedOperator()
{
}

operator_::Kind expression::UnresolvedOperator::kind() const
{
    return _kind;
}


expression_list expression::UnresolvedOperator::operands() const
{
    expression_list ops;

    for ( auto o : _ops )
        ops.push_back(o);

    return ops;
}

shared_ptr<Type> expression::UnresolvedOperator::type() const
{
    return std::make_shared<type::Unknown>(location());
}

expression::ResolvedOperator::ResolvedOperator(shared_ptr<Operator> op, const expression_list& ops, const Location& l)
    : CustomExpression(l)
{
    _op = op;

    for ( auto op : ops )
        _ops.push_back(op);

    for ( auto op : _ops )
        addChild(op);
}

expression::ResolvedOperator::~ResolvedOperator()
{
}

shared_ptr<Operator> expression::ResolvedOperator::operator_() const
{
    return _op;
}

operator_::Kind expression::ResolvedOperator::kind() const
{
    return _op->kind();
}

expression_list expression::ResolvedOperator::operands() const
{
    expression_list ops;

    for ( auto o : _ops )
        ops.push_back(o);

    return ops;
}

shared_ptr<Type> expression::ResolvedOperator::type() const
{
    expression_list ops;

    for ( auto o : _ops )
        ops.push_back(o);

    return _op->type(ops);
}

shared_ptr<Expression> expression::ResolvedOperator::op1() const
{
    return _ops.size() >= 1 ? _ops[0] : nullptr;
}

shared_ptr<Expression> expression::ResolvedOperator::op2() const
{
    return _ops.size() >= 2 ? _ops[1] : nullptr;
}

shared_ptr<Expression> expression::ResolvedOperator::op3() const
{
    return _ops.size() >= 3 ? _ops[2] : nullptr;
}

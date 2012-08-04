
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

Expression::Expression(const Location& l) : ast::Expression<AstInfo>(l)
{
}

bool Expression::initializer() const
{
    return false;
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

expression::UnresolvedOperator::UnresolvedOperator(binpac::operator_::Kind kind, const expression_list& ops, const Location& l)
    : Expression(l)
{
    _kind = kind;

    for ( auto op : ops )
        _ops.push_back(op);

    for ( auto op : _ops )
        addChild(op);
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

expression::ResolvedOperator::ResolvedOperator(shared_ptr<Operator> op, const expression_list& ops, const Location& l)
    : Expression(l)
{
    _op = op;

    for ( auto op : ops )
        _ops.push_back(op);

    for ( auto op : _ops )
        addChild(op);
}

shared_ptr<Operator> expression::ResolvedOperator::operator_() const
{
    return _op;
}

operator_::Kind expression::ResolvedOperator::kind() const
{
    return _op->kind();
}

const std::list<node_ptr<Expression>>& expression::ResolvedOperator::operands() const
{
    return _ops;
}

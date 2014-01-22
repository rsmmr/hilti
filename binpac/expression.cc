
#include "expression.h"
#include "module.h"

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

bool Expression::canCoerceTo(shared_ptr<Type> target) const
{
#if 0
    std::cerr << "canCoerceTo: "
              << const_cast<Expression *>(this)->render()
              << " " << type()->render()
              << " -> " << target->render()
              << std::endl;
#endif

    bool result = false;

    // TODO: We arent't using the expression constant coercion support here;
    // we should probably adapt the AST classes so that we can remove the
    // Coercer there (whihc however will break HILTI, which then should
    // implement data-type specific coercion in simialr way as we do here.)
    // And then this code should move there.
    auto const_ = dynamic_cast<const expression::Constant *>(this);

//    std::cerr << "    " << (const_ ? "is a constant" : "is not a constant") << std::endl;

    if ( const_ && ConstantCoercer().canCoerceTo(const_->constant(), target) ) {
//        std::cerr << "    const-coerce succeeds" << std::endl;
        result = true;
    }
    else
        result = Coercer().canCoerceTo(type(), target);

//    std::cerr << "    " << (result ? "true" : "false") << std::endl;
    return result;
}

shared_ptr<binpac::Expression> Expression::coerceTo(shared_ptr<Type> target)
{
#if 0
    if ( ! canCoerceTo(target) ) {
        std::cerr << util::fmt("debug check failed: cannot coerce expression of type %s to type %s (%s / %s)",
                               type()->render().c_str(),
                               target->render().c_str(),
                               render(), string(location())) << std::endl;
        abort();
    }
#endif

    auto t = type();

    if ( t->equal(target) || ast::isA<type::Any>(target) )
        return sharedPtr<binpac::Expression>();

    auto const_ = dynamic_cast<const expression::Constant *>(this);
    if ( const_ && ConstantCoercer().canCoerceTo(const_->constant(), target) )
        return std::make_shared<expression::Constant>(ConstantCoercer().coerceTo(const_->constant(), target));

    return std::make_shared<CoercedExpression>(sharedPtr<binpac::Expression>(), target, location());
}

expression::CustomExpression::CustomExpression(const Location& l) : Expression(l)
{
}

shared_ptr<Type> expression::CustomExpression::type() const
{
    // Must be overridden.
    assert(false);
    return nullptr;
}

bool expression::CustomExpression::isConstant() const
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

expression::CodeGen::CodeGen(shared_ptr<binpac::Type> type, shared_ptr<hilti::Expression> value, const Location& l)
    : CustomExpression(l)
{
    _type = type;
    _value = value;

    addChild(_type);
}

shared_ptr<hilti::Expression> expression::CodeGen::value() const
{
    return _value;
}

shared_ptr<Type> expression::CodeGen::type() const
{
    return _type;
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

expression::ParserState::ParserState(Kind kind, shared_ptr<binpac::ID> id, shared_ptr<Type> unit, shared_ptr<Type> type, const Location& l) : CustomExpression(l)
{
    _kind = kind;
    _id = id;
    _type = type;
    _unit = unit;

    addChild(_type);
    addChild(_id);
    addChild(_unit);
}

shared_ptr<Type> expression::ParserState::unit() const
{
    return _unit;
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
    if ( _type )
        return _type;

    // TODO: I believe we don't need the following code anymore now that we
    // have the unit-scope-builder pass. We should now give the type to the
    // parameter expressions there directly.

    assert(_kind == PARAMETER);

    auto utype = ast::tryCast<type::Unit>(_unit);

    if ( ! utype )
        return std::make_shared<type::Unknown>(location());

    for ( auto p : utype->parameters() ) {
        if ( *p->id() == *_id )
            return p->type();
    }

    return std::make_shared<type::Unknown>(location());
}

void expression::ParserState::setType(shared_ptr<binpac::Type> type)
{
    removeChild(_type);
    _type = (type ? type : std::make_shared<type::Unknown>(location()));
    addChild(_type);
}

expression::Assign::Assign(shared_ptr<Expression> dst, shared_ptr<Expression> src, const Location& l)
    : CustomExpression(l)
{
    _src = src;
    _dst = dst;
    addChild(_src);
    addChild(_dst);
}

shared_ptr<Expression> expression::Assign::source() const
{
    return _src;
}

shared_ptr<Expression> expression::Assign::destination() const
{
    return _dst;
}

shared_ptr<Type> expression::Assign::type() const
{
    return _dst->type();
}


expression::Conditional::Conditional(shared_ptr<Expression> cond, shared_ptr<Expression> true_, shared_ptr<Expression> false_, const Location& l)
{
    _cond = cond;
    _true = true_;
    _false = false_;
    addChild(_cond);
    addChild(_true);
    addChild(_false);
}

shared_ptr<Expression> expression::Conditional::condition() const
{
    return _cond;
}

shared_ptr<Expression> expression::Conditional::true_() const
{
    return _true;
}

shared_ptr<Expression> expression::Conditional::false_() const
{
    return _false;
}

shared_ptr<Type> expression::Conditional::type() const
{
    // We always return the type of the first operand here. The validator
    // makes sure that the two alternatives match.
    return _true->type();
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

expression::ResolvedOperator::ResolvedOperator(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<binpac::Module> module, const Location& l)
    : CustomExpression(l)
{
    _op = op;
    _module = module;

    for ( auto op : ops )
        _ops.push_back(op);

    for ( auto op : _ops )
        addChild(op);

    _type = nullptr;
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
    return _op->type(_module, operands());
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

expression::PlaceHolder::PlaceHolder(shared_ptr<binpac::Type> type, const Location& l)
    : CustomExpression(l)
{
    _type = type;
    addChild(_type);
}

shared_ptr<Type> expression::PlaceHolder::type() const
{
    return _type;
}


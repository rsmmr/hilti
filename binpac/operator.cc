
#include <stdlib.h>
#include <cxxabi.h>

#include "operator.h"
#include "expression.h"
#include "coercer.h"
#include "constant.h"
#include "statement.h"
#include "scope.h"

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

bool Operator::match(const expression_list& ops, bool coerce, expression_list* new_ops)
{
    std::list<shared_ptr<Type>> types = { __typeOp1(), __typeOp2(), __typeOp3() };

    auto o = ops.begin();
    auto t = types.begin();

    int i = 1;

    // fprintf(stderr, "\n");

    while ( o != ops.end() ) {
        // fprintf(stderr, "%s -> %s\n", (*o)->render().c_str(), (*t)->render().c_str());
        // fprintf(stderr, "  X %s\n", typeid(*(*o).get()).name());
        // fprintf(stderr, "  X %s\n", typeid(*(*o)->type().get()).name());

        if ( t == types.end() )
            // Too many arguments.
            return false;

        if ( coerce ) {
            if ( ! (*o)->canCoerceTo(*t) )
                return false;

            new_ops->push_back((*o)->coerceTo(*t));
        }

        else {
            assert((*o)->type());

            if ( ! (*o)->type()->equal(*t) )
                return false;

            new_ops->push_back(*o);
        }

        // fprintf(stderr, "  Match\n");

        ++o;
        ++t;
        ++i;
    }

    // All remaining operands must be options.
    while ( t != types.end() ) {
        if ( *t && ! ast::isA<type::OptionalArgument>(*t) )
            return false;

        ++t;
    }

    //  Sepcial handling for method calls: check the arguments.
    if ( _kind == operator_::MethodCall ) {
        pushOperands(ops);
        shared_ptr<binpac::Expression> op;

        if ( hasOp(3) )
            op = op3();

        else {
            expression_list empty_list;
            auto c = std::make_shared<constant::Tuple>(empty_list);
            op = std::make_shared<expression::Constant>(c);
        }

        if ( ! matchCallArgs(op, _callArgs()) )
            return false;

        popOperands();
    }

    // Match so far, now do any operand specific matching.
    pushOperands(ops);
    bool result = __match();
    popOperands();

    return result;
}

void Operator::validate(passes::Validator* vld, shared_ptr<Type> result, const expression_list& ops)
{
    _validator = vld;
    pushOperands(ops);
    __validate();
    popOperands();
    _validator = nullptr;

#if 0
    auto expected = type(ops);

    if ( ! Coercer().canCoerceTo(expected, result) )
        error(result, util::fmt("incompatible operator result (expected %s, but have %s)",
                                expected->render().c_str(), result->render().c_str()));
#endif
}

shared_ptr<Type> Operator::type(shared_ptr<Module> module, const expression_list& ops)
{
    pushOperands(ops);
    auto result = __typeResult();
    popOperands();

    auto name = ast::tryCast<type::TypeByName>(result);

    if ( name ) {
        auto id = name->id();
        auto types = module->body()->scope()->lookup(name->id(), true);

        if ( types.size() > 1 ) {
            error(name, util::fmt("ID %s defined more than once", id->pathAsString()));
            return result;
        }

        if ( types.size() == 1 ) {
            auto t = types.front();
            auto nt = ast::tryCast<expression::Type>(t);

            if ( ! nt ) {
                error(t, util::fmt("ID %s does not reference a type", id->pathAsString()));
                return result;
            }

            return nt->typeValue();
        }

        // We pass unknown types through, hoping they can be resolved later.
    }

    return result;
}

shared_ptr<Type> Operator::__typeOp3() const
{
    if ( _kind != operator_::MethodCall)
        return nullptr;
    else
        return std::make_shared<type::Tuple>();
}

bool Operator::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> type)
{
    if ( op->canCoerceTo(type) )
        return true;

    error(op, util::fmt("operand is of type %s but expected type", op->type(), type));
    return false;
}

bool Operator::sameType(shared_ptr<Type> type1, shared_ptr<Type> type2)
{
    if ( ! type1->equal(type2) ) {
        error(type1->firstParent<Node>(), "types do not match");
        return true;
    }

    return true;
}

bool Operator::matchesElementType(shared_ptr<Expression> element, shared_ptr<Type> container)
{
    auto ctype = ast::type::checkedTrait<type::trait::Container>(container);

    if ( ! element->canCoerceTo(ctype->elementType()) )
        error(element, "element type incompatible with container");

    return true;
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
    pushOperands(ops);
    __validate();
    popOperands();
    _validator = nullptr;
}

bool Operator::hasOp(int n)
{
    if ( operands().size() < n )
        return false;

    auto i = operands().begin();
    std::advance(i, n - 1);
    return (*i).get();
}

type_list Operator::_callArgs() const
{
    type_list types;

    if ( __typeCallArg1().second )
        types.push_back(__typeCallArg1().second);

    if ( __typeCallArg2().second )
        types.push_back(__typeCallArg2().second);

    if ( __typeCallArg3().second )
        types.push_back(__typeCallArg3().second);

    if ( __typeCallArg4().second )
        types.push_back(__typeCallArg4().second);

    if ( __typeCallArg5().second )
        types.push_back(__typeCallArg5().second);

    return types;
}

bool Operator::checkCallArgs(shared_ptr<Expression> tuple, const type_list& types)
{
    auto result = matchArgsInternal(tuple, types);
    if ( ! result.first )
        return true;

    error(result.first, result.second);
    return false;
}

bool Operator::matchCallArgs(shared_ptr<Expression> tuple, const type_list& types)
{
    auto result = matchArgsInternal(tuple, types);
    return (result.first == nullptr);
}

std::pair<shared_ptr<Node>, string> Operator::matchArgsInternal(shared_ptr<Expression> tuple, const type_list& types)
{
    if ( ! tuple ) {
        expression_list empty;
        auto c = std::make_shared<binpac::constant::Tuple>(empty);
        tuple = std::make_shared<expression::Constant>(c);
    }

    auto const_ = ast::checkedCast<expression::Constant>(tuple);
    auto elems = ast::checkedCast<constant::Tuple>(const_->constant())->value();

    if ( elems.size() > types.size() )
        return std::make_pair(tuple, "too many arguments");

    auto e = elems.begin();
    auto t = types.begin();

    while ( e != elems.end() ) {
        if ( ! (*e)->canCoerceTo(*t) )
            return std::make_pair(*e, "argument type mismatch");

        ++e;
        ++t;
    }

    // All remaining operands must be options.
    while ( t != types.end() ) {
        if ( *t && ! ast::isA<type::OptionalArgument>(*t) )
            return std::make_pair(tuple, "too many arguments");

        ++t;
    }

    return std::make_pair(nullptr, "");
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
    info.namespace_ = __namespace();
    info.kind_txt = i->second.kind_txt;
    info.description = __doc();
    info.render = render();
    info.type_op1 = __docTypeOp1();
    info.type_op2 = __docTypeOp2();
    info.type_op3 = __docTypeOp3();
    info.type_result = __docTypeResult();
    info.type_callarg1 = __docTypeCallArg1();
    info.type_callarg2 = __docTypeCallArg2();
    info.type_callarg3 = __docTypeCallArg3();
    info.type_callarg4 = __docTypeCallArg4();
    info.type_callarg5 = __docTypeCallArg5();
    info.class_op1 = _clsName(__typeOp1());
    info.class_op2 = _clsName(__typeOp2());
    info.class_op3 = _clsName(__typeOp3());
    return info;
}

OperatorRegistry::OperatorRegistry()
{
}

OperatorRegistry::~OperatorRegistry()
{
}

OperatorRegistry::matching_result OperatorRegistry::getMatching(operator_::Kind kind, const expression_list& ops, bool try_coercion, bool try_commutative) const
{
    matching_result matches;

    // We first try to find direct matches without coercing types. If that
    // finds something, we're done. Otherwise, we try coercion.

    auto m = _operators.equal_range(kind);

    for ( auto n = m.first; n != m.second; ++n ) {
        auto op = n->second;

        expression_list new_ops;
        if ( op->match(ops, false, &new_ops) )
            matches.push_back(std::make_pair(op, new_ops));
    }

    if ( matches.size() )
        return matches;

    if ( try_coercion ) {
        for ( auto n = m.first; n != m.second; ++n ) {
            auto op = n->second;

            expression_list new_ops;

            if ( op->match(ops, true, &new_ops) )
                matches.push_back(std::make_pair(op, new_ops));
        }
    }

    if ( matches.size() )
        return matches;

    // For commutative binary operators, try the reverse direction.
    auto opdef = operator_::OperatorDefinitions.find(kind);
    assert(opdef != operator_::OperatorDefinitions.end());

    if ( try_commutative && opdef->second.type == operator_::BINARY_COMMUTATIVE && ops.size() == 2 ) {
        auto i = ops.begin();
        auto op1 = *i++;
        auto op2 = *i++;

        expression_list new_ops = { op2, op1 };
        matches = getMatching(kind, new_ops, try_coercion, false);
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

shared_ptr<expression::ResolvedOperator> OperatorRegistry::resolveOperator(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<Module> module, const Location& l)
{
    return (op->factory())(op, ops, module, l);
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

    if ( op.type == operator_::BINARY || op.type == operator_::BINARY_COMMUTATIVE )
        return util::fmt("%s %s %s", op1->render(), op.display, op2->render());

    switch ( op.kind ) {
     case operator_::Add:
        return util::fmt("add %s[%s]", op1->render(), op2->render());

     case operator_::Attribute:
        return util::fmt("%s.%s", op1->render(), op2->render());

     case operator_::AttributeAssign:
        return util::fmt("%s.%s = %s", op1->render(), op2->render(), op3->render());

     case operator_::Call:
        return util::fmt("%s(%s)", op1->render(), op2->render());

     case operator_::Coerce:
        return util::fmt("(%s coerced to %s)", op1->render(), op2->render());

     case operator_::Cast:
        return util::fmt("cast<%s>(%s)", op2->render(), op1->render());

     case operator_::DecrPostfix:
        return util::fmt("%s--", op1->render());

     case operator_::Delete:
        return util::fmt("delete %s[%s]", op1->render(), op2->render());

     case operator_::HasAttribute:
        return util::fmt("%s?.%s", op1->render(), op2->render());

     case operator_::IncrPostfix:
        return util::fmt("%s++", op1->render());

     case operator_::Index:
        return util::fmt("%s[%s]", op1->render(), op2->render());

     case operator_::IndexAssign:
        return util::fmt("%s[%s] = %s", op1->render(), op2->render(), op3->render());

     case operator_::MethodCall: {
         std::list<string> args;
         for ( auto t : _callArgs() )
             args.push_back(t->render());

         return util::fmt("%s.%s(%s)", op1->render(), op2->render(), ::util::strjoin(args, ","));
     }

     case operator_::Size:
        return util::fmt("|%s|", op1->render());

     case operator_::TryAttribute:
        return util::fmt("%s.?%s", op1->render(), op2->render());

     default:
        fprintf(stderr, "unknown operator %d in Operator::render()", (int)op.kind);
        assert(false);
        return "";
    }
}



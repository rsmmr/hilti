
#ifndef BINPAC_EXPRESSION_H
#define BINPAC_EXPRESSION_H

#include <vector>

#include <ast/expression.h>

#include "common.h"
#include "operator.h"
#include "coercer.h"
#include "ctor.h"
#include "function.h"
#include "constant.h"
#include "constant-coercer.h"
#include "variable.h"
#include "module.h"

namespace binpac {

/// Base class for expression nodes.
class Expression : public ast::Expression<AstInfo>
{
public:
    /// Constructor.
    ///
    /// l: An associated location.
    Expression(const Location& l=Location::None);

    /// Returns true if the expressions value is suitable to initialize a
    /// variable/constant of corresponding type. The default implementation
    /// always returns false.
    virtual bool initializer() const;

    string render() override;

    operator string();

    ACCEPT_VISITOR_ROOT();
};

namespace expression {

/// Base class for expression nodes that aren't mixing in AST functionality.
///
/// This redefines the expression's virtual methods (which otherwise the
/// mix-ins provide).
class CustomExpression : public Expression
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    CustomExpression(const Location& l=Location::None);

    /// Must be overridden by childs.
    shared_ptr<Type> type() const override;

    /// Can be overridden. Implementation returns always false.
    bool isConstant() const override;

    // These use the standard coercer on the returned type().
    bool canCoerceTo(shared_ptr<Type> target) const override;
    shared_ptr<AIExpression> coerceTo(shared_ptr<Type> target) override;

    ACCEPT_VISITOR(Expression);
};

/// AST node for a list expressions.
class List : public binpac::Expression, public ast::expression::mixin::List<AstInfo>
{
public:
    /// Constructor.
    ///
    /// exprs: A list of individual expressions.
    ///
    /// l: An associated location.
    List(const expression_list& exprs, const Location& l=Location::None);

    ACCEPT_VISITOR(::binpac::Expression);
};

/// AST node for a constructor expression.
class Ctor : public binpac::Expression, public ast::expression::mixin::Ctor<AstInfo>
{
public:
    /// Constructor.
    ///
    /// values: The list of values representing the arguments to the constructor.
    ///
    /// type: The type of the constructed value.
    ///
    /// l: An associated location.
    Ctor(shared_ptr<binpac::Ctor> ctor, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a constant
class Constant : public binpac::Expression, public ast::expression::mixin::Constant<AstInfo>
{
public:
    /// Constructor.
    ///
    /// constant: The constant.
    ///
    /// l: An associated location.
    Constant(shared_ptr<binpac::Constant> constant, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a variable.
class Variable : public binpac::Expression, public ast::expression::mixin::Variable<AstInfo>
{
public:
    /// Constructor.
    ///
    /// var: The variable.
    ///
    /// l: An associated location.
    Variable(shared_ptr<binpac::Variable> var, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a type.
class Type : public binpac::Expression, public ast::expression::mixin::Type<AstInfo>
{
public:
    /// Constructor.
    ///
    /// type: The type.
    ///
    /// l: An associated location.
    Type(shared_ptr<binpac::Type> type, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a module.
class Module : public binpac::Expression, public ast::expression::mixin::Module<AstInfo>
{
public:
    /// Constructor.
    ///
    /// module: The module.
    ///
    /// l: An associated location.
    Module(shared_ptr<binpac::Module> module, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a function.
class Function : public binpac::Expression, public ast::expression::mixin::Function<AstInfo>
{
public:
    /// Constructor.
    ///
    /// func: The function.
    ///
    /// l: An associated location.
    Function(shared_ptr<binpac::Function> func, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a function parameter.
class Parameter : public binpac::Expression, public ast::expression::mixin::Parameter<AstInfo>
{
public:
    /// Constructor.
    ///
    /// param: The parameter.
    ///
    /// l: An associated location.
    Parameter(shared_ptr<binpac::type::function::Parameter> param, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing an ID.
class ID : public binpac::Expression, public ast::expression::mixin::ID<AstInfo>
{
public:
    /// Constructor.
    ///
    /// id: The ID.
    ///
    /// l: An associated location.
    ID(shared_ptr<binpac::ID> id, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression that represents another expression coerced to
/// a different type.
class Coerced : public binpac::Expression, public ast::expression::mixin::Coerced<AstInfo>
{
public:
    /// Constructor.
    ///
    /// expr: The original expression.
    ///
    /// dst: The type it is being coerced into. Note that the assumption is
    /// that the type coercion is legal.
    ///
    /// l: An associated location.
    Coerced(shared_ptr<binpac::Expression> expr, shared_ptr<binpac::Type> dst, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression that's used internal by the code generator.
class CodeGen : public binpac::Expression, public ast::expression::mixin::CodeGen<AstInfo>
{
public:
    /// Constructor.
    ///
    /// type: The type of the expression.
    ///
    /// cookie: A value left to the interpretation of the code generator.
    ///
    /// l: An associated location.
    CodeGen(shared_ptr<binpac::Type> type, void* cookie, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Expression);
};

/// AST node for an expression referencing a member attribute of a another
/// type.
class MemberAttribute : public CustomExpression
{
public:
    /// Constructor.
    ///
    /// attr: The attribute ID.
    ///
    /// l: An associated location.
    MemberAttribute(shared_ptr<binpac::ID> attr, const Location& l=Location::None);

    /// Returns the expression member attribute ID.
    shared_ptr<binpac::ID> id() const;

    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(binpac::Expression);

private:
    node_ptr<binpac::ID> _attribute;
    node_ptr<binpac::Type> _type;
};

/// AST node for an expression that references internal parser state. This is
/// used for reserved IDs like \a $$ and \a self. The expression's type() is
/// initially unknown until resolved later.
class ParserState : public CustomExpression
{
public:
    enum Kind {
        SELF,         /// A \a self expression.
        DOLLARDOLLAR, /// A \a $$ expression.
        PARAMETER     /// A type parameter, with \a id giving its name. 
    };

    /// Constructor.
    ///
    /// kind: The kind of expression (i.e., the specific value the expression references).
    ///
    /// id: An optional ID associated with the state reference. Depends on
    /// the kind whether it's needed.
    ///
    /// l: An associated location.
    ParserState(Kind kind, shared_ptr<binpac::ID> id = nullptr, shared_ptr<Type> type = nullptr, const Location& l=Location::None);

    /// Return the kind of expression.
    Kind kind() const;

    /// Returns the associated ID, if set.
    shared_ptr<binpac::ID> id() const;

    /// Sets the expression's type. This is intended to be used from the
    /// resolver only.
    void setType(shared_ptr<binpac::Type> type);

    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(binpac::Expression);

private:
    Kind _kind;
    node_ptr<binpac::ID> _id;
    node_ptr<binpac::Type> _type;
};

/// A not yet resolved (and potentially overloaded) operator. Initially, all
/// operators are instantiated as Unresolved and later turned into instances
/// derived from Resolved by passes::OperandResolver.
///
class UnresolvedOperator : public CustomExpression
{
public:
    /// Constructor.
    ///
    /// kind: The operator kind.
    ///
    /// ops: The list of operands.
    ///
    /// l: An associated location.
    UnresolvedOperator(binpac::operator_::Kind kind, const expression_list& ops, const Location& l=Location::None);
    virtual ~UnresolvedOperator();

    /// Returns the operator type.
    binpac::operator_::Kind kind() const;

    expression_list operands() const;

    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(binpac::Expression);

private:
    binpac::operator_::Kind _kind;
    std::list<node_ptr<Expression>> _ops;
};

/// Base class for uniquenly resolved operators.
class ResolvedOperator : public CustomExpression
{
public:
    /// Constructor.
    ///
    /// op: The operator.
    ///
    /// ops: The list of operands.
    ///
    /// l: An associated location.
    ResolvedOperator(shared_ptr<Operator> op, const expression_list& ops, const Location& l=Location::None);
    ~ResolvedOperator();

    /// Returns the operator.
    shared_ptr<Operator> operator_() const;

    /// Returns the first operator, or null if none.
    shared_ptr<Expression> op1() const;

    /// Returns the second operator, or null if none.
    shared_ptr<Expression> op2() const;

    /// Returns the third operator, or null if none.
    shared_ptr<Expression> op3() const;

    /// Returns the operator type.
    binpac::operator_::Kind kind() const;

    /// Returns the operands.
    expression_list operands() const;

    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(binpac::Expression);

protected:

private:
    shared_ptr<Operator> _op;
    std::vector<node_ptr<Expression>> _ops;
};

}

}

#endif

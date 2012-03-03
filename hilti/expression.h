
#ifndef HILTI_EXPRESSION_H
#define HILTI_EXPRESSION_H

#include "common.h"
#include "id.h"
#include "coercer.h"
#include "ctor.h"
#include "constant-coercer.h"
#include "scope.h"
#include "variable.h"

namespace hilti {

/// Base class for expression nodes.
class Expression : public ast::Expression<AstInfo>
{
public:
   /// Constructor.
   ///
   /// l: An associated location.
   Expression(const Location& l=Location::None) : ast::Expression<AstInfo>(l) {}

   /// Returns a readable one-line representation of the expression.
   string render() override;

   ACCEPT_VISITOR_ROOT();
};

namespace expression {

/// AST node for a list expressions.
class List : public hilti::Expression, public ast::expression::mixin::List<AstInfo>
{
public:
   /// Constructor.
   ///
   /// exprs: A list of individual expressions.
   ///
   /// l: An associated location.
   List(const expr_list& exprs, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::List<AstInfo>(this, exprs) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for a constructor expression.
class Ctor : public hilti::Expression, public ast::expression::mixin::Ctor<AstInfo>
{
public:
   /// Constructor.
   ///
   /// values: The list of values representing the arguments to the constructor.
   ///
   /// type: The type of the constructed value.
   ///
   /// l: An associated location.
   Ctor(shared_ptr<hilti::Ctor> ctor, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Ctor<AstInfo>(this, ctor) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a constant
class Constant : public hilti::Expression, public ast::expression::mixin::Constant<AstInfo>
{
public:
   /// Constructor.
   ///
   /// constant: The constant.
   ///
   /// l: An associated location.
   Constant(shared_ptr<hilti::Constant> constant, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Constant<AstInfo>(this, constant) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a variable.
class Variable : public hilti::Expression, public ast::expression::mixin::Variable<AstInfo>
{
public:
   /// Constructor.
   ///
   /// var: The variable.
   ///
   /// l: An associated location.
   Variable(shared_ptr<hilti::Variable> var, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Variable<AstInfo>(this, var) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a type.
class Type : public hilti::Expression, public ast::expression::mixin::Type<AstInfo>
{
public:
   /// Constructor.
   ///
   /// type: The type.
   ///
   /// l: An associated location.
   Type(shared_ptr<hilti::Type> type, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Type<AstInfo>(this, type) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a statement block.
class Block : public hilti::Expression, public ast::expression::mixin::Block<AstInfo>
{
public:
   /// Constructor.
   ///
   /// block: The block.
   ///
   /// l: An associated location.
   Block(shared_ptr<hilti::statement::Block> block, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Block<AstInfo>(this, block) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a module.
class Module : public hilti::Expression, public ast::expression::mixin::Module<AstInfo>
{
public:
   /// Constructor.
   ///
   /// module: The module.
   ///
   /// l: An associated location.
   Module(shared_ptr<hilti::Module> module, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Module<AstInfo>(this, module) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a function.
class Function : public hilti::Expression, public ast::expression::mixin::Function<AstInfo>
{
public:
   /// Constructor.
   ///
   /// func: The function.
   ///
   /// l: An associated location.
   Function(shared_ptr<hilti::Function> func, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Function<AstInfo>(this, func) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing a function parameter.
class Parameter : public hilti::Expression, public ast::expression::mixin::Parameter<AstInfo>
{
public:
   /// Constructor.
   ///
   /// param: The parameter.
   ///
   /// l: An associated location.
   Parameter(shared_ptr<hilti::type::function::Parameter> param, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Parameter<AstInfo>(this, param) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression referencing an ID.
class ID : public hilti::Expression, public ast::expression::mixin::ID<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: The ID.
   ///
   /// l: An associated location.
   ID(shared_ptr<hilti::ID> id, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::ID<AstInfo>(this, id) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression that represents another expression coerced to
/// a different type.
class Coerced : public hilti::Expression, public ast::expression::mixin::Coerced<AstInfo>
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
   Coerced(shared_ptr<hilti::Expression> expr, shared_ptr<hilti::Type> dst, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::Coerced<AstInfo>(this, expr, dst) {}

   ACCEPT_VISITOR(hilti::Expression);
};

/// AST node for an expression that's used internal by the code generator.
class CodeGen : public hilti::Expression, public ast::expression::mixin::CodeGen<AstInfo>
{
public:
   /// Constructor.
   ///
   /// type: The type of the expression.
   ///
   /// cookie: A value left to the interpretation of the code generator.
   ///
   /// l: An associated location.
   CodeGen(shared_ptr<hilti::Type> type, void* cookie, const Location& l=Location::None)
       : hilti::Expression(l), ast::expression::mixin::CodeGen<AstInfo>(this, type, cookie) {}

   ACCEPT_VISITOR(hilti::Expression);
};

}

}

#endif

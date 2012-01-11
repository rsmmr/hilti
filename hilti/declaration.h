
#ifndef HILTI_DECLARATION_H
#define HILTI_DECLARATION_H

#include "common.h"
#include "id.h"
#include "type.h"
#include "expression.h"

namespace hilti {

/// Base class for AST declaration nodes.
class Declaration : public ast::Declaration<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: The name of declared object.
   /// 
   /// l: An associated location.
   Declaration(shared_ptr<hilti::ID> id, const Location& l=Location::None) : ast::Declaration<AstInfo>(id, l) {}

   ACCEPT_VISITOR_ROOT();
};

namespace declaration {

/// AST node for declaring a variable.
class Variable : public hilti::Declaration, public ast::declaration::mixin::Variable<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: The name of declared variable.
   /// 
   /// var: The declared variable.
   ///
   /// l: An associated location.
   Variable(shared_ptr<hilti::ID> id, shared_ptr<hilti::Variable> var, const Location& l=Location::None)
       : hilti::Declaration(id, l), ast::declaration::mixin::Variable<AstInfo>(this, var) {}

   ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a constant.
class Constant : public hilti::Declaration, public ast::declaration::mixin::Constant<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: The name of declared constant.
   /// 
   /// constant: The declared constant.
   ///
   /// l: An associated location.
   Constant(shared_ptr<hilti::ID> id, shared_ptr<hilti::Constant> constant, const Location& l=Location::None)
       : hilti::Declaration(id, l), ast::declaration::mixin::Constant<AstInfo>(this, constant) {}

   ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a type.
class Type : public hilti::Declaration, public ast::declaration::mixin::Type<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: The name of declared type.
   /// 
   /// type: The declared type.
   ///
   /// l: An associated location.
   Type(shared_ptr<hilti::ID> id, shared_ptr<hilti::Type> type, const Location& l=Location::None)
       : hilti::Declaration(id, l), ast::declaration::mixin::Type<AstInfo>(this, type) {}

   ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a function.
class Function : public hilti::Declaration, public ast::declaration::mixin::Function<AstInfo>
{
public:
   /// Constructor.
   ///
   /// id: The name of declared function.
   /// 
   /// func: The declared function.
   ///
   /// l: An associated location.
   Function(shared_ptr<hilti::ID> id, shared_ptr<hilti::Function> func, const Location& l=Location::None)
       : hilti::Declaration(id, l), ast::declaration::mixin::Function<AstInfo>(this, func) {}

   ACCEPT_VISITOR(hilti::Declaration);
};

}

}

#endif

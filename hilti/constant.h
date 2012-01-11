
#ifndef HILTI_CONSTANT_H
#define HILTI_CONSTANT_H

#include <stdint.h>

#include "common.h"
#include "scope.h"
#include "type.h"
#include "visitor-interface.h"

namespace hilti {

/// Base class for constant nodes.
class Constant : public ast::Constant<AstInfo>
{
public:
   /// Constructor.
   ///
   /// l: An associated location.
   Constant(const Location& l=Location::None)
       : ast::Constant<AstInfo>(l) {}

   ACCEPT_VISITOR_ROOT();
};

namespace constant {

/// AST node for a constant of type String.
class String : public ast::SpecificConstant<AstInfo, Constant, string>
{
public:
   /// Constructor.
   ///
   /// value: The value of the constant.
   ///
   /// l: An associated location.
   String(string value, const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, string>(value, shared_ptr<Type>(new type::String()), l) {}

   ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Integer.
class Integer : public ast::SpecificConstant<AstInfo, Constant, int64_t>
{
public:
   /// Constructor.
   ///
   /// value: The value of the constant.
   ///
   /// width: The bit width of the integer type.
   ///
   /// l: An associated location.
   Integer(int64_t value, int width, const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, int64_t>(value, shared_ptr<Type>(new type::Integer(width)), l) {}

   ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Bool.
class Bool : public ast::SpecificConstant<AstInfo, Constant, bool>
{
public:
   /// Constructor.
   ///
   /// value: The value of the constant.
   ///
   /// l: An associated location.
   Bool(bool value, const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, bool>(value, shared_ptr<Type>(new type::Bool()), l) {}

   ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Unset.
class Unset : public ast::SpecificConstant<AstInfo, Constant, bool>
{
public:
   /// Constructor.
   ///
   /// l: An associated location.
   Unset(const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, bool>(false, shared_ptr<Type>(new type::Unset()), l) {}
};

/// AST node for a constant of type Tuple.
class Tuple : public Constant
{
public:
   typedef std::list<node_ptr<Expression>> element_list;

   /// Constructor.
   ///
   /// elem: The constant's elements.
   ///
   /// l: An associated location.
   Tuple(const element_list& elems, const Location& l=Location::None);

   /// Returns a list of the constant's elements.
   const element_list& value() const { return _elems; }

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override;

   ACCEPT_VISITOR(Constant);

private:
   element_list _elems;
};


}

}

#endif

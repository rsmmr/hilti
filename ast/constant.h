
#ifndef AST_CONSTANT_H
#define AST_CONSTANT_H

#include "exception.h"
#include "node.h"
#include "visitor.h"

namespace ast {

/// Base class for AST nodes representing constant values.
template<typename AstInfo>
class Constant : public AstInfo::node
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::visitor_interface VisitorInterface;

   // Constructor.
   //
   /// l: A location associated with the constant.
   Constant(const Location& l=Location::None)
       : AstInfo::node(l) {}

   /// Returns the type of the constant. Must be overriden by derived
   /// classes.
   virtual shared_ptr<Type> type() const = 0;

   ACCEPT_DISABLED;
};


/// A constant of a non-composite value. This is a helper template that
/// defines constant of atomic values, such as integers or strings.
///
/// Do not use it for composite types as it will not resolve subtypes
/// correctly.
template<typename AstInfo, typename Base, typename Value>
class SpecificConstant : public Base
{
public:
   typedef typename AstInfo::node Node;
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::visitor_interface VisitorInterface;

   /// Constructor. Creates a new constant of the given value and type.
   ///
   /// value: The constant's value.
   ///
   /// type: The constant's type.
   ///
   /// l: A location associated with the node.
   SpecificConstant(Value value, shared_ptr<Type> type, const Location& l=Location::None)
       : Base(l) {
           _value = value;
           _type = type;
           this->addChild(_type);
       }

   /// Returns the constant's value.
   Value value() const { return _value; }

   /// Returns the constant's type.
   shared_ptr<Type> type() const { return _type; } // FIXME: "override" doesn't work?

private:
   node_ptr<Type> _type;
   Value _value;
};


}

#endif

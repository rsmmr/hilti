
#ifndef AST_TYPE_H
#define AST_TYPE_H

#include <util/util.h>

#include "node.h"
#include "id.h"
#include "mixin.h"

namespace ast {

class Trait {};
class Constable {};
class Constructable {};
class Parameterizable {};

template<class AstInfo>
class Type;

/// Base class for mix-ins that want to override some of a type's virtual
/// methods. See Type for documentation.
template<class AstInfo>
class TypeOverrider : public Overrider<typename AstInfo::type> {
public:
   /// See Type::repr().
   virtual string repr() const { assert(false); }
};

/// Base class for AST nodes representing types.
template<class AstInfo>
class Type : public AstInfo::node, public Overridable<TypeOverrider<AstInfo>>
{
public:
   typedef typename AstInfo::node Node;
   typedef typename AstInfo::type AIType;
   typedef typename AstInfo::id ID;

   /// Constructor.
   ///
   /// l: A location associated with the expression.
   Type(Location l=Location::None) : AstInfo::node(l) {}

   /// Returns true if two types are equivalent.
   ///
   /// These rules are applied:
   ///
   ///    - Types marked with setWildcard() match any other type of the same
   ///    Type class.
   ///
   ///    - Types marked with setAny() match any other type.
   ///
   ///    - If the two types are of the the same type class, the result of
   ///      _equal() is passed on (which defaults to true).
   ///
   ///    - Otherwise, comparision fails.
   virtual bool equal(shared_ptr<AIType> other) const {
       if ( _any || other->_any )
           return true;

       if ( typeid(*this) == typeid(*other) ) {
           if ( _wildcard || other->_wildcard )
               return true;

           return _equal(other);
       }

       return false;
   }

   /// Compares two type instances of the same Type class. The default
   /// implementation returns true, but can be overridden.
   ///
   /// other: The instance to compare with. It's guaranteed that this is of
   /// the same type as \c *this.
   ///
   /// Note, when overriding, make sure to keep comparision commutative.
   virtual bool _equal(shared_ptr<AIType> other) const { return true; }

   /// Returns the ID associated with the type, or null of none.
   shared_ptr<ID> id() const { return _id; }

   /// Associated an ID with the type.
   void setID(shared_ptr<ID> id) {
       if ( _id )
           this->removeChild(_id);

       _id = id;
       this->addChild(_id);
   }

   /// Returns true if the type has been marked a wildcard type. See
   /// setWildcard().
   bool wildcard(void) const { return _wildcard; }

   /// Marks the type as wildcard type. When comparing types with
   /// operator==(), a wilcard type matches all other instances of the same
   /// Type class.
   void setWildcard(bool wildcard) { _wildcard = wildcard; }

   /// Returns true if the type has been marked to match any other. See
   /// setMatchesAny().
   bool matchesAny(void) const { return _any; }

   /// Marks the type as matching any other. When comparing types with
   /// operator==(), such a type will always match anything else.
   void setMatchesAny(bool any) { _any = any; }

private:
   bool operator==(const Type& other) const; // Disable.

   node_ptr<ID> _id;
   bool _wildcard = false;
   bool _any = false;
};

namespace type {

namespace mixin {

// Short-cut to save some typing.
#define __TYPE_MIXIN ::ast::MixIn<typename AstInfo::type, typename ::ast::TypeOverrider<AstInfo>>

namespace function {

/// A helper class for mixin::Function to describe a function paramemeter or
/// return value.
template<typename AstInfo>
class Parameter : public AstInfo::node
{
public:
   typedef typename AstInfo::node Node;
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::id ID;

   /// Constructor for function parameters.
   ///
   /// id: The name of the parameter.
   ///
   /// type: The type of the parameter.
   ///
   /// constant: A flag indicating whether the parameter is constant (i.e., a
   /// function invocation won't change its value.)
   ///
   /// default_value: An optional default value for the parameters, or null if none.
   ///
   /// l: A location associated with the expression.
   Parameter(shared_ptr<ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value = nullptr, Location l=Location::None)
       : Node(l) { Init(id, type, constant, default_value); }

   /// Constructor for a return value.
   ///
   /// type: The type of the return value.
   ///
   /// constant: A flag indicating whether the return value is constant
   /// (i.e., a caller may not change its value.)
   ///
   /// l: A location associated with the expression.
   Parameter(shared_ptr<Type> type, bool constant, Location l=Location::None)
       : Node(l) { Init(nullptr, type, constant); }

   /// Returns the name of the parameter. Null for return values.
   shared_ptr<ID> id() const { return _id; }

   /// Returns true if the parameter is marked constant.
   bool constant() const { return _constant; }

   /// Returns the type of the parameter.
   shared_ptr<Type> type() const { return _type; }

   /// Returns the default value of the parameter, or null if none.
   shared_ptr<Expression> defaultValue() const { return _default_value; }

   /// Returns a string representation of the parameter for type comparision.
   string repr() const {
       auto id = _id ? (string(" ") + _id->pathAsString()).c_str() : "";
       return util::fmt("%s%s%s", (_constant ? "const " : ""), _type->render().c_str(), id);
   }

   bool operator==(const Parameter& other) { return repr() == other.repr(); }
   bool operator!=(const Parameter& other) { return repr() != other.repr(); }

private:
   void Init(shared_ptr<ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value = nullptr);

   node_ptr<ID> _id;
   bool _constant;
   node_ptr<Type> _type;
   node_ptr<Expression> _default_value;

};

template<typename AstInfo>
inline void Parameter<AstInfo>::Init(shared_ptr<ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value)
{
    _id = id;
    _constant = constant;
    _type = type;
    _default_value = default_value;

    if ( _id )
        this->addChild(_id);

    this->addChild(_type);
    this->addChild(_default_value);
}

}

/// A mix-in class to define a function type.
template<typename AstInfo>
class Function : public __TYPE_MIXIN, public TypeOverrider<AstInfo>
{
public:
   typedef typename AstInfo::node Node;
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::parameter Parameter;

   typedef std::list<node_ptr<Parameter>> parameter_list;

   /// Constructor.
   ///
   /// target: The type we're mixed in with.
   ///
   /// result: The function return type.
   ///
   /// params: The function's parameters.
   Function(Type* target, shared_ptr<Parameter> result, const parameter_list& params);

   /// Returns the function's parameters.
   const parameter_list& parameters() const { return _params; }

   /// Returns the function's result type.
   shared_ptr<Parameter> result() const { return _result; }

   /// Returns true if a list of expressions is compatiable with the
   /// function's parameters.
   template<typename IterableExpression>
   bool matchParameters(const IterableExpression& values) {
       auto a = _params.begin();

       for ( auto e : values ) {
           if ( a == _params.end() )
               return false;

           if ( ! e.canCoerceTo(a.type) )
               return false;

           if ( a->constant() && ! e->isConstant() )
               return false;
       }

       // Left-over parameters must have a default value.
       while ( a++ != _params.end() ) {
           if ( ! a->default_value )
               return false;
       }

       return true;
   }

   /// Returns true if a list an expression is compatiable with the
   /// function's return type.
   bool matchResult(shared_ptr<Expression> e) {
       if ( ! e.canCoerceTo(_result.type()) )
           return false;

       return true;
   }

   string repr() const /* override*/;

protected:
   parameter_list _params;
   node_ptr<Parameter> _result;
};

}

//////////////////////// Implementations.

template<typename AstInfo>
inline mixin::Function<AstInfo>::Function(Type* target, shared_ptr<Parameter> result, const parameter_list& params)
    : __TYPE_MIXIN(target, this)
{
    _params = params;
    _result = result;

    target->addChild(_result);
    for ( auto a : params ) {
        target->addChild(a);
    }
}

template<typename AstInfo>
string mixin::Function<AstInfo>::repr() const {
   string s = _result->render() + string(" (");

   bool first = true;
   for ( auto a : _params ) {
       if ( ! first )
           s += ", ";

       s += a->repr();

       first = false;
   }

   s += ")";

   return s;
}

}

}

#endif

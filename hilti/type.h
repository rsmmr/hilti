
#ifndef HILTI_TYPE_H
#define HILTI_TYPE_H

// TODO: Much more of this should move into ast/type.h

#include "common.h"
#include "visitor-interface.h"

using namespace hilti;

namespace hilti {

/// Base class for all AST nodes representing a type.
class Type : public ast::Type<AstInfo>
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Type(const Location& l=Location::None) : ast::Type<AstInfo>(l) {}
   ACCEPT_VISITOR_ROOT();
};

namespace type {

namespace trait {

// Base class for type trait. We use trait to mark types that have certain properties.
//
// Note that Traits aren't (and can't) be derived from Node and thus aren't AST nodes.
class Trait
{
public:
   Trait() {}
   virtual ~Trait();
};

namespace parameter {

/// Base class for a type parameter. This is used with trait::Parameterized.
class Base : public Node {
public:
   virtual ~Base() {}

   /// Returns a string representation of the parameters. Can be used from a
   /// Node::repr().
   virtual string repr() const = 0;

   ACCEPT_VISITOR_ROOT();
};

/// A type parameter representing a Type.
class Type : public Base {
public:
   /// Constructor.
   ///
   /// type: The type the parameter specifies.
   Type(shared_ptr<hilti::Type> type) { _type = type; addChild(_type); }

   /// Returns the type the parameter specifies.
   shared_ptr<hilti::Type> type() const { return _type; }

   string repr() const override;

   ACCEPT_VISITOR(Base);

private:
   node_ptr<hilti::Type> _type;
};

/// A type parameter representing an integer value.
class Integer : public Base {
public:
   /// Constructor.
   ///
   /// value: The parameter's integer value.
   Integer(int64_t value) { _value = value; }

   /// Returns the parameters integer value.
   int64_t value() const { return _value; }

   string repr() const override { return ::util::fmt("%d", _value); }

   ACCEPT_VISITOR(Base);

private:
   int64_t _value;
};

}

/// Trait class marking a type with parameters.
class Parameterized : public Trait
{
public:
   typedef std::list<node_ptr<parameter::Base>> parameter_list;

   /// Returns the type's parameters.
   virtual const std::list<node_ptr<parameter::Base>>& parameters() const = 0;

   /// Returns a prefix for a string representation for the parameterized
   /// type. repr() will return ``prefix<p1,p2,...>``.
   virtual string reprPrefix() const = 0;

   string repr() const;
};

/// Trait class marking a composite type that has a series of subtypes.
class TypeList : public Trait
{
public:
   typedef std::list<node_ptr<hilti::Type>> type_list;

   /// Returns the ordered list of subtypes.
   virtual const type_list& typeList() const = 0;
};

/// Trait class marking types which's instances are garbage collected by the
/// HILTI run-time.
class GarbageCollected : public Trait
{
};

// class Iterable.
// class Unpackable.
// class Classifiable.
// class Blockable.

}

/// Returns true if type \a t has trait \a T.
template<typename T>
inline bool hasTrait(const hilti::Type* t) { return dynamic_cast<const T*>(t) != 0; }

/// Returns true if type \a t has trait \a T.
template<typename T>
inline bool hasTrait(shared_ptr<hilti::Type> t) { return std::dynamic_pointer_cast<T>(t) != 0; }

/// Dynamic-casts type \a t into trait class \a T.
template<typename T>
inline T* asTrait(hilti::Type* t) { return dynamic_cast<T*>(t); }

/// Dynamic-casts type \a t into trait class \a T.
template<typename T>
inline const T* asTrait(const hilti::Type* t) { return dynamic_cast<const T*>(t); }

/// Dynamic-casts type \a t into trait class \a T.
template<typename T>
inline T* asTrait(shared_ptr<hilti::Type> t) { return std::dynamic_pointer_cast<T>(t); }

/// Base class for all of HILTI's type.s
class Type : public hilti::Type
{
public:
   Type() {}
   virtual ~Type();
   virtual string repr() const { return "<Type>"; }
   ACCEPT_VISITOR(Type);
};

/// Base class for types that a user can instantiate.
class HiltiType : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   HiltiType(const Location& l=Location::None) : Type(l) {}

};

/// Base class for types that are stored on the stack and value-copied.
class ValueType : public HiltiType
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   ValueType(const Location& l=Location::None) : HiltiType(l) {}

};

/// Base class for types that stored on the heap and manipulated only by
/// reference.
class HeapType : public HiltiType, public trait::GarbageCollected
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   HeapType(const Location& l=Location::None) : HiltiType(l) {}

};

/// A tupe matching any other type.
class Any : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Any(const Location& l=Location::None) : hilti::Type(l) { setMatchesAny(true); }
   virtual ~Any();
   virtual string repr() const { return "<Any>"; }
   ACCEPT_VISITOR(Type);
};

/// A place holder type representing a type that has not been resolved yet.
class Unknown : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Unknown(const Location& l=Location::None) : hilti::Type(l) {}
   virtual ~Unknown();
   virtual string repr() const { return "<Unknown>"; }
   ACCEPT_VISITOR(Type);
};

/// A type representing an unset value.
class Unset : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Unset(const Location& l=Location::None) : hilti::Type(l) {}
   virtual ~Unset();
   virtual string repr() const { return "<Unset>"; }
   ACCEPT_VISITOR(Type);
};

/// A type representing a statement::Block.
class Block : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Block(const Location& l=Location::None) : hilti::Type(l)  {}
   virtual ~Block();
   virtual string repr() const { return "<Block>"; }
   ACCEPT_VISITOR(Block);
};

/// A type representing a Module.
class Module : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Module(const Location& l=Location::None) : hilti::Type(l) {}
   virtual ~Module();
   virtual string repr() const { return "<Module>"; }
   ACCEPT_VISITOR(Module);
};

/// A type representing a non-existing return value.
class Void : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Void(const Location& l=Location::None) : hilti::Type(l) {}
   virtual ~Void();
   virtual string repr() const { return "void"; }
   ACCEPT_VISITOR(Type);
};

/// Type for strings.
class String : public ValueType, public trait::GarbageCollected {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   String(const Location& l=Location::None) : ValueType(l) {}
   virtual ~String();

   virtual string repr() const { return "string"; }
   ACCEPT_VISITOR(Type);
};

/// Type for boolean values.
class Bool : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Bool(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Bool();

   virtual string repr() const { return "bool"; }
   ACCEPT_VISITOR(Type);
};

/// Type for integer values.
class Integer : public ValueType, public trait::Parameterized {
public:
   /// Constructor.
   ///
   /// width: The bit width for the type.
   ///
   /// l: Associated location.
   Integer(int width, const Location& l=Location::None) : ValueType(l) {
       _width = width;
       auto p = shared_ptr<trait::parameter::Base>(new trait::parameter::Integer(width));
       _parameters.push_back(p);
       }

   virtual ~Integer();

   /// Returns the types bit width.
   int width() const { return _width; }

   string reprPrefix() const override { return "int"; }
   string repr() const override { return trait::Parameterized::repr(); }
   const std::list<node_ptr<trait::parameter::Base>>& parameters() const override { return _parameters; }

   ACCEPT_VISITOR(Type);

private:
   int _width;
   parameter_list _parameters;
};

/// Type for tuples.
class Tuple : public ValueType, public trait::Parameterized, public trait::TypeList {
public:
   typedef trait::TypeList::type_list type_list;

   /// Constructor.
   ///
   /// types: The types of the tuple's elements.
   ///
   /// l: Associated location.
   Tuple(const type_list& types, const Location& l=Location::None);

   /// Constructor for an empty tuple.
   ///
   /// l: Associated location.
   Tuple(const Location& l=Location::None);
   
   const type_list& typeList() const override;
   string reprPrefix() const override { return "tuple"; }
   string repr() const override { return trait::Parameterized::repr(); }
   const std::list<node_ptr<trait::parameter::Base>>& parameters() const override { return _parameters; }

   ACCEPT_VISITOR(Type);

private:
   parameter_list _parameters;
   type_list _types;
};

namespace function {

/// Helper type to define a function parameter or return value.
class Parameter : public ast::type::mixin::function::Parameter<AstInfo>
{
public:
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
   Parameter(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l=Location::None);

   /// Constructor for a return value.
   ///
   /// type: The type of the return value.
   ///
   /// constant: A flag indicating whether the return value is constant
   /// (i.e., a caller may not change its value.)
   ///
   /// l: A location associated with the expression.
   Parameter(shared_ptr<Type> type, bool constant, Location l=Location::None);

   ACCEPT_VISITOR_ROOT();
};

typedef ast::type::mixin::Function<AstInfo>::parameter_list parameter_list;

/// HILTI's supported calling conventions.
enum CallingConvention {
    DEFAULT,    /// Place-holder for chosing a default automatically.
    HILTI,      /// Internal convention for calls between HILTI functions. 
    HILTI_C,    /// C-compatible calling convention, but with extra HILTI run-time parameters.
    C           /// C-compatable calling convention, with no messing around with parameters.
};

}

/// Type for functions.
class Function : public hilti::Type, public ast::type::mixin::Function<AstInfo>  {
public:
   /// Constructor.
   ///
   /// target: The type we're mixed in with.
   ///
   /// result: The function return type.
   ///
   /// params: The function's parameters.
   ///
   /// cc: The function's calling convention.
   ///
   /// l: Associated location.
   Function(shared_ptr<hilti::type::function::Parameter> result, const function::parameter_list& args, hilti::type::function::CallingConvention cc, const Location& l=Location::None);

   /// Returns the function's calling convention.
   hilti::type::function::CallingConvention callingConvention() const { return _cc; }

   ACCEPT_VISITOR(hilti::Type);

private:
   hilti::type::function::CallingConvention _cc;
};

}

}

#endif

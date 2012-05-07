
#ifndef HILTI_TYPE_H
#define HILTI_TYPE_H

// TODO: Much more of this should move into ast/type.h

#include <vector>

#include "common.h"
#include "scope.h"
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

   /// Returns a readable one-line representation of the type.
   string render() override;

   /// Returns a scope of sub-identifiers that a relative to the type. If a
   /// type defines IDs in this scope and a global type declaration is added
   /// to a module, these IDs will be accessible via scoping relative to the
   /// declaration. The returned scope should not have its parent set.
   virtual shared_ptr<Scope> typeScope() { return nullptr; }

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
class Base {
public:
   virtual ~Base() {}

   /// Will be called to compare this node against another of the *same*
   /// type.
   virtual bool _equal(shared_ptr<Base> other) const = 0;
};

/// A type parameter representing a Type.
class Type : public Base {
public:
   /// Constructor.
   ///
   /// type: The type the parameter specifies.
   Type(shared_ptr<hilti::Type> type) { _type = type; }

   /// Returns the type the parameter specifies.
   shared_ptr<hilti::Type> type() const { return _type; }

   bool _equal(shared_ptr<Base> other) const override {
       return _type->equal(std::dynamic_pointer_cast<type::trait::parameter::Type>(other)->_type);
   }

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

   bool _equal(shared_ptr<Base> other) const override {
       return _value == std::dynamic_pointer_cast<Integer>(other)->_value;
   }

private:
   int64_t _value;
};

/// A type parameter representing an enum value. This must include the scope
/// for a module-level lookup.
class Enum : public Base {
public:
   /// Constructor.
   ///
   /// label: The parameter's enum identifier.
   Enum(shared_ptr<ID> label) { _label = label; }

   /// Returns the parameters enum identifier.
   shared_ptr<ID> label() const { return _label; }

   bool _equal(shared_ptr<Base> other) const override {
       return _label->pathAsString() == std::dynamic_pointer_cast<Enum>(other)->_label->pathAsString();
   }

private:
   shared_ptr<ID> _label;
};

/// A type parameter representing an attribute (e.g., \c &abc).
class Attribute : public Base {
public:
   /// Constructor.
   ///
   /// value: The parameter's attribute, including the ampersand.
   Attribute(const string& attr) { _attr = attr; }

   /// Returns the attribute.
   const string& value() const { return _attr; }

   bool _equal(shared_ptr<Base> other) const override {
       return _attr == std::dynamic_pointer_cast<Attribute>(other)->_attr;
   }

private:
   string _attr;
};

}

/// Trait class marking a type with parameters.
class Parameterized : public Trait
{
public:
   typedef std::list<shared_ptr<parameter::Base>> parameter_list;

   /// Compares the type with another for equivalence, which must be an
   /// instance of the same type class.
   bool equal(shared_ptr<hilti::Type> other) const;

   /// Returns the type's parameters. Note we return this by value as the
   /// derived classes need to build the list on the fly each time so that
   /// potential updates to the members (in particular type resolving) get
   /// reflected.
   virtual parameter_list parameters() const = 0;
};

/// Trait class marking a composite type that has a series of subtypes.
class TypeList : public Trait
{
public:
   typedef std::list<shared_ptr<hilti::Type>> type_list;

   /// Returns the ordered list of subtypes.
   virtual const type_list typeList() const = 0;
};

/// Trait class marking types which's instances are garbage collected by the
/// HILTI run-time.
class GarbageCollected : public Trait
{
};

/// Trait class marking a type that provides iterators.
class Iterable : public Trait
{
public:
   /// Returns the type for an iterator over this type.
   virtual shared_ptr<hilti::Type> iterType() = 0;
};

/// Trait class marking a type that can be hashed for storing in containers.
class Hashable : public Trait
{
public:
};

/// Trait class marking a container type.
class Container : public Trait, public Iterable
{
};

/// Trait class marking types which's instances can be unpacked from binary
/// data via the Unpacker.
class Unpackable : public Trait
{
public:
   /// Description of an unpack format supported by the type.
   struct Format {
       /// The fully-qualified name of a ``Hilti::Packed`` constant (i.e.,
       /// ``Hilti::Packed::Bool``.
       string enumName;

       /// The type of the unpacked element.
       shared_ptr<hilti::Type> result_type;

       /// The type for the 2nd unpack argument, or null if not used.
       shared_ptr<hilti::Type> arg_type;

       /// True if the 2nd argument is optional and may be skipped.
       bool arg_optional;

       /// Description of the format suitable for inclusion in documentation.
       string doc;
   };

   /// Returns the suportted unpack formats.
   virtual const std::vector<Format>& unpackFormats() const = 0;
};

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

/// An interal Type-derived class that allows us to specify an optional
/// parameters in an instruction's signature.
class OptionalArgument : public hilti::Type
{
public:
   OptionalArgument(shared_ptr<Type> arg) { _arg = arg; }

   string render() override {
       return string("[ " + _arg->render() + " ]");
   }

   bool equal(shared_ptr<hilti::Type> other) const override {
       return other ? _arg->equal(other) : true;
   }

   shared_ptr<hilti::Type> argType() const { return _arg; }

private:
   shared_ptr<Type> _arg;
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

/// Base class for a heap type parameterized with a single type parameter.
class TypedHeapType : public HeapType, public trait::Parameterized {
public:
   /// Constructor.
   ///
   /// argtype: The type parameter.
   ///
   /// l: Associated location.
   TypedHeapType(shared_ptr<Type> argtype, const Location& l=Location::None);

   /// Constructor for a wildcard type.
   ///
   /// l: Associated location.
   TypedHeapType(const Location& l=Location::None);

   shared_ptr<Type> argType() const { return _argtype; }

   parameter_list parameters() const override;

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

private:
   node_ptr<Type> _argtype;
};

/// Base class for a value type parameterized with a single type parameter.
class TypedValueType : public ValueType, public trait::Parameterized {
public:
   /// Constructor.
   ///
   /// argtype: The type parameter.
   ///
   /// l: Associated location.
   TypedValueType(shared_ptr<Type> argtype, const Location& l=Location::None);

   /// Constructor for a wildcard type.
   ///
   /// l: Associated location.
   TypedValueType(const Location& l=Location::None);

   shared_ptr<Type> argType() const { return _argtype; }

   parameter_list parameters() const override;

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

private:
   node_ptr<Type> _argtype;
};

/// Base type for iterators.
class Iterator : public TypedValueType
{
public:
   /// Constructor for a wildcard iterator type.
   ///
   /// l: Associated location.
   Iterator(const Location& l=Location::None) : TypedValueType(l) {}

   ACCEPT_VISITOR(Type);

protected:
   /// Constructor for an iterator over a given target tyoe.
   ///
   /// ttype: The target type. This must be a trait::Iterable.
   ///
   /// l: Associated location.
   ///
   Iterator(shared_ptr<Type> ttype, const Location& l=Location::None) : TypedValueType(ttype, l) {}

private:
   node_ptr<hilti::Type> _ttype;
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
   ACCEPT_VISITOR(Type);
};

/// A place holder type representing a type that has not been resolved yet.
class Unknown : public hilti::Type
{
public:
   /// Constructor for a type not further specified. This must be resolved by
   /// some external means.
   ///
   /// l: Associated location.
   Unknown(const Location& l=Location::None) : hilti::Type(l) {}

   /// Constructor referencing a type by its name. This will be resolved
   /// automatically by resolver.
   Unknown(shared_ptr<ID> id, const Location& l=Location::None) : hilti::Type(l) { _id = id; addChild(_id); }

   /// If an ID is associated with the type, returns it; null otherwise.
   shared_ptr<ID> id() const { return _id; }

   virtual ~Unknown();

   ACCEPT_VISITOR(Type);

private:
   node_ptr<ID> _id = nullptr;
};

/// An internal place holder type representing a type by its name, to be
/// resolved later. This should be used only in situations where's its
/// guaranteed that the resolving will be done manually, there's no automatic
/// handling.
class TypeByName : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// name: The type name.
   ///
   /// l: Associated location.
   TypeByName(const string& name, const Location& l=Location::None) : hilti::Type(l) {
       _name = name;
   }
   virtual ~TypeByName();

   const string& name() const { return _name; }

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return std::dynamic_pointer_cast<TypeByName>(other)->name() == name();
   }

   ACCEPT_VISITOR(Type);

private:
   string _name;
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
   ACCEPT_VISITOR(Type);
};

/// A type representing the name of an instruction block.
class Label : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Label(const Location& l=Location::None) : hilti::Type(l) {}
   virtual ~Label();
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

   ACCEPT_VISITOR(Type);
};

/// Type for IP addresses.
class Address : public ValueType, public trait::Unpackable {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Address(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Address();

   const std::vector<Format>& unpackFormats() const override;

   ACCEPT_VISITOR(Type);
};

/// Type for IP subnets.
class Network : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Network(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Network();

   ACCEPT_VISITOR(Type);
};

/// Type for ports.
class Port : public ValueType, public trait::Unpackable {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Port(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Port();

   const std::vector<Format>& unpackFormats() const override;

   ACCEPT_VISITOR(Type);
};

/// Type for bitsets.
class Bitset : public ValueType {
public:
   typedef std::pair<shared_ptr<ID>, int> Label;
   typedef std::list<Label> label_list;

   /// Constructor.
   ///
   /// labels: List of pairs of label name and bit number. If a bit number is
   /// -1, it will be chosen automaticallty.
   ///
   /// l: Associated location.
   Bitset(const label_list& labels, const Location& l=Location::None);
   virtual ~Bitset();

   /// Constructor creating a bitset type matching any other bitset type.
   Bitset(const Location& l=Location::None) : ValueType(l) {
       setWildcard(true);
   }

   // Returns the labels with their bit numbers.
   const label_list& labels() const { return _labels; }

   // Returns the bit number for a label, which must be known.
   int labelBit(shared_ptr<ID> label) const;

   shared_ptr<Scope> typeScope() override;

   bool _equal(shared_ptr<hilti::Type> other) const override;

   ACCEPT_VISITOR(Type);

private:
   label_list _labels;
   shared_ptr<Scope> _scope = nullptr;
};

/// Type for enums.
class Enum : public ValueType {
public:
   typedef std::pair<shared_ptr<ID>, int> Label;
   typedef std::list<Label> label_list;

   /// Constructor.
   ///
   /// labels: List of pairs of label name and value. If a bit number is -1,
   /// it will be chosen automaticallty.
   ///
   /// l: Associated location.
   Enum(const label_list& labels, const Location& l=Location::None);
   virtual ~Enum();

   /// Constructor creating an enum type matching any other enum type.
   Enum(const Location& l=Location::None) : ValueType(l) {
       setWildcard(true);
   }

   // Returns the labels with their bit numbers.
   const label_list& labels() const { return _labels; }

   // Returns the value for a label, which must be known. Returns -1 for \c Undef.
   int labelValue(shared_ptr<ID> label) const;

   shared_ptr<Scope> typeScope() override;

   bool _equal(shared_ptr<hilti::Type> other) const override;

   ACCEPT_VISITOR(Type);

private:
   label_list _labels;
   shared_ptr<Scope> _scope = nullptr;
};

/// Type for caddr values.
class CAddr : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   CAddr(const Location& l=Location::None) : ValueType(l) {}
   virtual ~CAddr();

   ACCEPT_VISITOR(Type);
};

/// Type for doubles.
class Double : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Double(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Double();

   ACCEPT_VISITOR(Type);
};

/// Type for booleans.
class Bool : public ValueType, public trait::Unpackable {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Bool(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Bool();

   const std::vector<Format>& unpackFormats() const override;

   ACCEPT_VISITOR(Type);
};

/// Type for interval values.
class Interval : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Interval(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Interval();

   ACCEPT_VISITOR(Type);
};

/// Type for time values.
class Time : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Time(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Time();

   ACCEPT_VISITOR(Type);
};


/// Type for integer values.
class Integer : public ValueType, public trait::Parameterized, public trait::Unpackable
{
public:
   /// Constructor.
   ///
   /// width: The bit width for the type.
   ///
   /// l: Associated location.
   Integer(int width, const Location& l=Location::None) : ValueType(l) {
       _width = width;
       }

   /// Constructore creating an integer type matching any other integer type (i.e., \c int<*>).
   Integer(const Location& l=Location::None) : ValueType(l) {
       _width = 0;
       setWildcard(true);
   }

   virtual ~Integer();

   /// Returns the types bit width.
   int width() const { return _width; }

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

   parameter_list parameters() const override;

   const std::vector<Format>& unpackFormats() const override;

   ACCEPT_VISITOR(Type);

private:
   int _width;
};

/// Type for tuples.
class Tuple : public ValueType, public trait::Parameterized, public trait::TypeList {
public:
   typedef std::list<shared_ptr<hilti::Type>> type_list;

   /// Constructor.
   ///
   /// types: The types of the tuple's elements.
   ///
   /// l: Associated location.
   Tuple(const type_list& types, const Location& l=Location::None);

   /// Constructor for a wildcard tuple type matching any other.
   ///
   /// l: Associated location.
   Tuple(const Location& l=Location::None);

   const trait::TypeList::type_list typeList() const override;
   parameter_list parameters() const override;

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

   ACCEPT_VISITOR(Type);

private:
   std::list<node_ptr<hilti::Type>> _types;
};

/// Type for types.
class Type : public hilti::Type
{
public:
   /// Constructor.
   ///
   /// type; The represented type.
   ///
   /// l: Associated location.
   Type(shared_ptr<hilti::Type> type, const Location& l=Location::None) : hilti::Type(l) {
       _rtype = type; addChild(_rtype);
   }

   /// Constructor for wildcard type.
   ///
   /// l: Associated location.
   Type(const Location& l=Location::None) : hilti::Type() {
       setWildcard(true);
   }

   ~Type();

   shared_ptr<hilti::Type> typeType() const { return _rtype; }

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return _rtype->equal(std::dynamic_pointer_cast<Type>(other)->_rtype);
   }

   ACCEPT_VISITOR(hilti::Type);

private:
   node_ptr<hilti::Type> _rtype;
};

/// Type for exceptions.
class Exception : public TypedHeapType
{
public:
   /// Constructor.
   ///
   /// base: Another exception type this one derived from. If null, the
   /// top-level base exception is used.
   ///
   /// arg: The type of the exceptions argument, or null if none.
   ///
   /// l: Location associated with the type.
   Exception(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l=Location::None);
   ~Exception();

   /// Constructor. This creates a wildcard type matching any other exception type.
   Exception(const Location& l=Location::None) : TypedHeapType(l) {}

   /// Returns the base exception type this one is derived from. Returns null
   /// for the top-level base exception.
   shared_ptr<Type> baseType() const { return _base; }

   /// Sets the base exception type this one is derived from.
   void setBaseType(shared_ptr<Type> base) { _base = base; }

   /// Returns true if this is the top-level root type.
   bool isRootType() const { return _libtype == "hlt_exception_unspecified"; }

   /// Marks this type as defined by libhilti under the given name. Setting
   /// an empty name unmarks.
   void setLibraryType(const string& name) { _libtype = name; }

   /// If the type is defined in libhilti, returns the name of the internal
   /// global. If not, returns an empty string.
   string libraryType() const { return _libtype; }

   ACCEPT_VISITOR(Type);

private:
   node_ptr<Type> _base = nullptr;
   string _libtype;
};

/// Type for references.
class Reference : public TypedValueType
{
public:
   /// Constructor.
   ///
   /// rtype: The referenced type. This must be a HeapType.
   ///
   /// l: Associated location.
   Reference(shared_ptr<Type> rtype, const Location& l=Location::None) : TypedValueType(rtype, l) {}

   /// Constructor for wildcard reference type..
   ///
   /// l: Associated location.
   Reference(const Location& l=Location::None) : TypedValueType(l) {}

   ACCEPT_VISITOR(Type);
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
    HOOK,       /// Internal convention for hooks.
    HILTI_C,    /// C-compatible calling convention, but with extra HILTI run-time parameters.
    C           /// C-compatable calling convention, with no messing around with parameters.
};

}

/// Type for functions.
class Function : public hilti::Type, public ast::type::mixin::Function<AstInfo>  {
public:
   /// Constructor.
   ///
   /// result: The function return type.
   ///
   /// params: The function's parameters.
   ///
   /// cc: The function's calling convention.
   ///
   /// l: Associated location.
   Function(shared_ptr<hilti::type::function::Parameter> result, const function::parameter_list& args, hilti::type::function::CallingConvention cc, const Location& l=Location::None);

   /// Constructor for a function type that matches any other function type (i.e., a wildcard type).
   Function(const Location& l=Location::None);

   /// Returns the function's calling convention.
   hilti::type::function::CallingConvention callingConvention() const { return _cc; }

   /// Sets the function's calling convention.
   void setCallingConvention(hilti::type::function::CallingConvention cc) { _cc = cc; }

   bool _equal(shared_ptr<hilti::Type> other) const override;

   ACCEPT_VISITOR(hilti::Type);

private:
   hilti::type::function::CallingConvention _cc;
};

/// Type for hooks.
class Hook : public Function
{
public:
   /// Constructor.
   ///
   /// result: The hook's return type.
   ///
   /// params: The hooks's parameters.
   ///
   /// l: Associated location.
   Hook(shared_ptr<hilti::type::function::Parameter> result, const function::parameter_list& args, const Location& l=Location::None)
       : Function(result, args, function::HOOK, l) {}

   /// Constructor for a hook type that matches any other hook type (i.e., a
   /// wildcard type).
   Hook(const Location& l=Location::None) : Function(l) {}

   virtual ~Hook();
};

/// Type for bytes instances.
///
class Bytes : public HeapType, public trait::Iterable, public trait::Unpackable
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Bytes(const Location& l=Location::None) : HeapType(l) {}
   virtual ~Bytes();

   shared_ptr<hilti::Type> iterType() override;

   const std::vector<Format>& unpackFormats() const override;

   ACCEPT_VISITOR(Type);
};

namespace iterator {

class Bytes : public Iterator
{
public:
   /// Constructor for an iterator over a bytes object.
   ///
   /// l: Associated location.
   ///
   Bytes(const Location& l=Location::None) : Iterator(shared_ptr<Type>(new type::Bytes(l))) {}

   ACCEPT_VISITOR(Iterator);
};

}

/// Type for callable instances.
///
class Callable : public TypedHeapType
{
public:
   /// Constructor.
   ///
   /// rtype: The callable's return type. This must be a value type.
   ///
   /// l: Associated location.
   Callable(shared_ptr<Type> rtype, const Location& l=Location::None) : TypedHeapType(rtype, l) {}

   /// Constructor for wildcard callable type..
   ///
   /// l: Associated location.
   Callable(const Location& l=Location::None) : TypedHeapType(l) {}

   /// Destructor.
   ~Callable();

   ACCEPT_VISITOR(Type);
};

/// Type for classifier instances.
///
class Classifier : public HeapType
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Classifier(const Location& l=Location::None) : HeapType(l) {}
   virtual ~Classifier();

   ACCEPT_VISITOR(Type);
};

/// Type for file instances.
///
class File : public HeapType
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   File(const Location& l=Location::None) : HeapType(l) {}
   virtual ~File();

   ACCEPT_VISITOR(Type);
};

/// Type for channels.
class Channel : public TypedHeapType
{
public:
   /// Constructor.
   ///
   /// type: The channel's data type. This must be a ValueType.
   ///
   /// l: Associated location.
   Channel(shared_ptr<Type> type, const Location& l=Location::None) : TypedHeapType(type, l) {}

   /// Constructor for wildcard reference type..
   ///
   /// l: Associated location.
   Channel(const Location& l=Location::None) : TypedHeapType(l) {}

   virtual ~Channel();
   
   ACCEPT_VISITOR(Type);
};

/// Type for IOSource instances.
class IOSource : public HeapType, public trait::Parameterized, public trait::Iterable
{
public:
   /// Constructor.
   ///
   /// kind: An enum identifier referencing the type of IOSource (``Hilti::IOSrc::*``).
   ///
   /// l: Associated location.
   IOSource(shared_ptr<ID> kind, const Location& l=Location::None) : HeapType(l) {
       _kind = kind;
       addChild(_kind);
   }

   /// Constructore creating an IOSrc type matching any other (i.e., \c iosrc<*>).
   IOSource(const Location& l=Location::None) : HeapType(l) {
       setWildcard(true);
   }

   virtual ~IOSource();

   /// Returns the enum label for the type of IOSource.
   shared_ptr<ID> kind() const { return _kind; }

   shared_ptr<hilti::Type> iterType() override;

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

   parameter_list parameters() const override;

   ACCEPT_VISITOR(Type);

private:
   node_ptr<ID> _kind;
};

namespace iterator {

class IOSource : public Iterator
{
public:
   /// Constructor for an iterator over an IOSource object.
   ///
   /// ty: The IOSource type to iterator over.
   ///
   /// l: Associated location.
   ///
   IOSource(shared_ptr<Type> ty, const Location& l=Location::None)
       : Iterator(ty, l) {}

   /// Constructor for an iterator over a widlcard IOSource object.
   ///
   /// l: Associated location.
   ///
   IOSource(const Location& l=Location::None)
       : IOSource { std::make_shared<type::IOSource>(l) } {}

   ACCEPT_VISITOR(Iterator);
};

}

namespace iterator {

/// A template class for defining container iterators.
template<typename T>
class ContainerIterator : public Iterator
{
protected:
   /// Constructor for an iterator over a container type.
   ///
   /// ctype: The container type this iterator iterates over.
   ///
   /// l: Associated location.
   ///
   ContainerIterator(shared_ptr<Type> ctype, const Location& l=Location::None) : Iterator(ctype) {}
};

class Map : public ContainerIterator<Map>
{
public:
   /// FIXMEL This should be "using
   /// ContainerIterator<Map>::ContainterIterator; ...", but clang doesn't
   /// like it yet.
   Map(shared_ptr<Type> ctype, const Location& l=Location::None) : ContainerIterator<Map>(ctype, l) {}
   ACCEPT_VISITOR(Iterator);
};

class Set : public ContainerIterator<Set>
{
public:
   /// FIXMEL This should be "using
   /// ContainerIterator<Set>::ContainterIterator; ...", but clang doesn't
   /// like it yet.
   Set(shared_ptr<Type> ctype, const Location& l=Location::None) : ContainerIterator<Set>(ctype, l) {}
   ACCEPT_VISITOR(Iterator);
};

class Vector : public ContainerIterator<Vector>
{
public:
   /// FIXMEL This should be "using
   /// ContainerIterator<Vector>::ContainterIterator; ...", but clang doesn't
   /// like it yet.
   Vector(shared_ptr<Type> ctype, const Location& l=Location::None) : ContainerIterator<Vector>(ctype, l) {}
   ACCEPT_VISITOR(Iterator);
};

class List : public ContainerIterator<List>
{
public:
   /// FIXMEL This should be "using
   /// ContainerIterator<List>::ContainterIterator; ...", but clang doesn't
   /// like it yet.
   List(shared_ptr<Type> ctype, const Location& l=Location::None) : ContainerIterator<List>(ctype, l) {}
   ACCEPT_VISITOR(Iterator);
};


}

/// Type for list objects.
class List : public TypedHeapType, public trait::Container
{
public:
   /// Constructor.
   ///
   /// etype: The type of the container's elements.
   ///
   /// l: Associated location.
   List(shared_ptr<Type> etype, const Location& l=Location::None)
       : TypedHeapType(etype, l) {}

   /// Constructor for a wildcard container type.
   List(const Location& l=Location::None) : TypedHeapType(l) {}

   virtual ~List();

   shared_ptr<hilti::Type> iterType() override {
       return std::make_shared<iterator::List>(this->sharedPtr<Type>(), location());
   }

   ACCEPT_VISITOR(Type);
};

/// Type for vector objects.
class Vector : public TypedHeapType, public trait::Container
{
public:
   /// Constructor.
   ///
   /// etype: The type of the container's elements.
   ///
   /// l: Associated location.
   Vector(shared_ptr<Type> etype, const Location& l=Location::None)
       : TypedHeapType(etype, l) {}

   /// Constructor for a wildcard container type.
   Vector(const Location& l=Location::None) : TypedHeapType(l) {}

   virtual ~Vector();

   shared_ptr<hilti::Type> iterType() override {
       return std::make_shared<iterator::List>(this->sharedPtr<Type>(), location());
   }

   ACCEPT_VISITOR(Type);
};

/// Type for vector objects.
class Set : public TypedHeapType, public trait::Container
{
public:
   /// Constructor.
   ///
   /// etype: The type of the container's elements.
   ///
   /// l: Associated location.
   Set(shared_ptr<Type> etype, const Location& l=Location::None)
       : TypedHeapType(etype, l) {}

   /// Constructor for a wildcard container type.
   Set(const Location& l=Location::None) : TypedHeapType(l) {}

   virtual ~Set();

   shared_ptr<hilti::Type> iterType() override {
       return std::make_shared<iterator::List>(this->sharedPtr<Type>(), location());
   }

   ACCEPT_VISITOR(Type);
};

/// Type for map objects.
class Map : public HeapType, public trait::Parameterized, public trait::Container
{
public:
   /// Constructor.
   ///
   /// key: The type of the container's index.
   ///
   /// value: The type of the container's values.
   ///
   /// l: Associated location.
   Map(shared_ptr<Type> key, shared_ptr<Type> value, const Location& l=Location::None)
       : HeapType(l) { _key = key; _value = value; addChild(_key); addChild(_value); }

   /// Constructor for a wildcard map type.
   Map(const Location& l=Location::None) : HeapType(l) {}

   virtual ~Map();

   shared_ptr<hilti::Type> iterType() override {
       return std::make_shared<iterator::Map>(this->sharedPtr<Type>(), location());
   }

   shared_ptr<Type> keyType() const { return _key; }
   shared_ptr<Type> valueType() const { return _value; }

   parameter_list parameters() const override;

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

   ACCEPT_VISITOR(Type);

private:
   node_ptr<hilti::Type> _key = nullptr;
   node_ptr<hilti::Type> _value = nullptr;
};

/// Type for overlays.
///
/// \todo Implementation missing.
class  Overlay : public ValueType {
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Overlay(const Location& l=Location::None) : ValueType(l) {}
   virtual ~Overlay();

   ACCEPT_VISITOR(Type);
};

/// Type for regexp instances.
class RegExp : public HeapType, public trait::Parameterized {
public:
   typedef std::list<string> attribute_list;

   /// Constructor.
   ///
   /// attrs: List of optional attributes controlling specifics of the
   /// regular expression. These must include the ampersand, e.g., \c &nosub.
   ///
   /// l: Associated location.
   RegExp(const attribute_list& attrs, const Location& l=Location::None) : HeapType(l) {
       _attrs = attrs;
       }

   /// Constructore creating an IOSrc type matching any other (i.e., \c regexp<*>).
   RegExp(const Location& l=Location::None) : HeapType(l) {
       setWildcard(true);
   }

   virtual ~RegExp();

   /// Returns the enum label for the type of RegExp.
   const attribute_list& attributes() const { return _attrs; }

   bool _equal(shared_ptr<hilti::Type> other) const override {
       return trait::Parameterized::equal(other);
   }

   parameter_list parameters() const override;

   ACCEPT_VISITOR(Type);

private:
   attribute_list _attrs;
};

/// Type for \c match_token_state
///
class MatchTokenState : public HeapType
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   MatchTokenState(const Location& l=Location::None) : HeapType(l) {}
   virtual ~MatchTokenState();

   ACCEPT_VISITOR(Type);
};

/// Type for a timer object.
///
class Timer : public HeapType
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   Timer(const Location& l=Location::None) : HeapType(l) {}
   virtual ~Timer();

   ACCEPT_VISITOR(Type);
};

/// Type for a timer_mgr object.
///
class TimerMgr : public HeapType
{
public:
   /// Constructor.
   ///
   /// l: Associated location.
   TimerMgr(const Location& l=Location::None) : HeapType(l) {}
   virtual ~TimerMgr();

   ACCEPT_VISITOR(Type);
};

namespace struct_ {

/// Definition of one struct field.
class Field : public Node
{
public:
   /// id:  The name of the field.
   ///
   /// type: The type of the field.
   ///
   /// default_: An optional default value, null if no default.
   ///
   /// internal: If true, the field will not be printed when the struct
   /// type is rendered as a string. Internal IDS are also skipped from
   /// ctor expressions and list conversions.
   ///
   /// l: Location associated with the field.
   Field(shared_ptr<ID> id, shared_ptr<hilti::Type> type, shared_ptr<Expression> default_=nullptr, bool internal=false, const Location& l=Location::None);

   shared_ptr<ID> id() const { return _id; }
   shared_ptr<Type> type() const { return _type; }
   shared_ptr<Expression> default_() const;
   bool internal() const { return _internal; }

   ACCEPT_VISITOR_ROOT();

private:
   node_ptr<ID> _id;
   node_ptr<Type> _type;
   node_ptr<Expression> _default;
   bool _internal;
};


}

/// Type for structs.
class Struct : public ValueType, public trait::TypeList
{
public:
   typedef std::list<node_ptr<struct_::Field>> field_list;

   /// Constructor.
   ///
   /// fields: The struct's fields.
   ///
   /// l: Associated location.
   Struct(const field_list& fields, const Location& l=Location::None);

   /// Constructor for a wildcard struct type matching any other.
   ///
   /// l: Associated location.
   Struct(const Location& l=Location::None);

   virtual ~Struct();

   /// Returns the list of fields.
   const field_list& fields() const { return _fields; }

   const trait::TypeList::type_list typeList() const override;

   bool _equal(shared_ptr<hilti::Type> other) const override;

   ACCEPT_VISITOR(Type);

private:
   std::list<node_ptr<struct_::Field>> _fields;
};

////

}

}

#endif

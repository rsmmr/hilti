
#ifndef BINPAC_TYPE_H
#define BINPAC_TYPE_H

#include <ast/type.h>

#include "common.h"
#include "id.h"

using namespace binpac;

namespace binpac {

namespace passes { class GrammarBuilder; }

namespace type {

namespace trait {

typedef ast::type::Trait Trait;

// Generic trait borrowed from AST.

namespace parameter {
    typedef ast::type::trait::parameter::Type<AstInfo> Type;
    typedef ast::type::trait::parameter::Integer Integer;
    typedef ast::type::trait::parameter::Enum<AstInfo> Enum;
    typedef ast::type::trait::parameter::Attribute Attribute;
}

typedef ast::type::trait::Parameterized<AstInfo> Parameterized;
typedef ast::type::trait::TypeList<AstInfo> TypeList;
typedef ast::type::trait::Iterable<AstInfo> Iterable;
typedef ast::type::trait::Hashable Hashable;
typedef ast::type::trait::Container<AstInfo> Container;

// BinPAC++ specific traits.

/// Trait class marking a type that can be parsed from an input stream. Only
/// these types can be used inside a ~~Unit.
class Parseable : public trait::Trait
{
public:
    /// Returns the type of values directly parsed by this type. This type
    /// will become the type of the corresponding Unit item. While usually
    /// the parsed type is the same as the type itself (and that's what the
    /// default implementation returns), it does not need to. The RegExp type
    /// returns Bytes for example.
    ///
    /// The default implementation returns just \a this.
    ///
    /// Returns: The type for parsed items.
    virtual shared_ptr<binpac::Type> fieldType();


    /// A description of a custom type attribute, as returned by
    /// parseAttributes*().
    struct ParseAttribute {
        string key;                      /// The attribute name.
        shared_ptr<Type> type;           /// The attribute's expression type, or null for no argument.
        shared_ptr<Expression> default_; /// A default value for the expression if none is given, or null for no default.
        bool implicit_;                  /// If true and *default* is given, the attribute is implicitly assumed to be present even if not given.
     };

    /// Returns the custom attributes the type supports for parsing. Note
    /// that these are added on top of any attributes supported generally by
    /// all types (e.g., \a &default).
    ///
    /// The default implementation returns the empty list.
    virtual std::list<ParseAttribute> parseAttributes() const;
};


/// Trait class marking a type to which a Sink can directly be attached.
class Sinkable : public trait::Trait
{
};

}

}

/// Base class for all AST nodes representing a type.
class Type : public ast::Type<AstInfo>
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Type(const Location& l=Location::None);

    /// Returns a readable one-line representation of the type.
    string render() override;

    /// Returns a readable represention meant to be used in the automatically
    /// generated reference documentation. By default, this return the same as
    /// render() but can be overridden by derived classes.
    virtual string docRender();

    /// Returns a scope of sub-identifiers that are relative to the type. If a
    /// type defines IDs in this scope and a global type declaration is added
    /// to a module, these IDs will be accessible via scoping relative to the
    /// declaration. The returned scope should not have its parent set.
    virtual shared_ptr<Scope> typeScope();

    ACCEPT_VISITOR_ROOT();
};

namespace type {

/// An interal Type-derived class that allows us to specify optional
/// parameters in operator signatures.
class OptionalArgument : public binpac::Type
{
public:
    OptionalArgument(shared_ptr<Type> arg);

    /// Returns the argument, or null if not given.
    shared_ptr<binpac::Type> argType() const;

    string render() override;
    bool equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    shared_ptr<Type> _arg;
};

/// Base class for instantiable BinPAC++ types.
class PacType : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    PacType(const Location& l=Location::None);

    /// Constructor.
    ///
    /// attrs: Attributes associated with the type.
    ///
    /// l: Associated location.
    PacType(const attribute_list& attrs,  const Location& l=Location::None);

#if 0
    /// Returns the attributes associated with the type.
    ///
    /// \todo: Actually I don't think that types should have attributes.
    shared_ptr<AttributeSet> attributes() const;
#endif

    ACCEPT_VISITOR(Type);

protected:
    node_ptr<AttributeSet> _attrs;
};

/// Base class for instatiable types parameterized with a single type parameter.
class TypedPacType : public PacType, public trait::Parameterized
{
public:
    /// Constructor.
    ///
    /// argtype: The type parameter.
    ///
    /// l: Associated location.
    TypedPacType(shared_ptr<Type> argtype, const Location& l=Location::None);

    /// Constructor for a wildcard type.
    ///
    /// l: Associated location.
    TypedPacType(const Location& l=Location::None);

    /// Returns the type's parameter.
    shared_ptr<Type> argType() const;

    type_parameter_list parameters() const override;
    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    node_ptr<Type> _argtype;
};

/// Base type for iterators.
class Iterator : public TypedPacType
{
public:
    /// Constructor for a wildcard iterator type.
    ///
    /// l: Associated location.
    Iterator(const Location& l=Location::None);

    /// Constructor for an iterator over a given target tyoe.
    ///
    /// ttype: The target type. This must be a trait::Iterable.
    ///
    /// l: Associated location.
    Iterator(shared_ptr<Type> ttype, const Location& l=Location::None);

    ACCEPT_VISITOR(TypedPacType);

private:
    node_ptr<Type> _ttype;
};

/// A tupe matching any other type.
class Any : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Any(const Location& l=Location::None);

    ACCEPT_VISITOR(Type);
};

/// A place holder type representing a type that has not been resolved yet.
class Unknown : public binpac::Type
{
public:
    /// Constructor for a type not further specified. This must be resolved by
    /// some external means.
    ///
    /// l: Associated location.
    Unknown(const Location& l=Location::None);

    /// Constructor referencing a type by its name. This will be resolved
    /// automatically by resolver.
    Unknown(shared_ptr<ID> id, const Location& l=Location::None);

    /// If an ID is associated with the type, returns it; null otherwise.
    shared_ptr<ID> id() const;

    ACCEPT_VISITOR(Type);

private:
    node_ptr<ID> _id = nullptr;
};

/// An internal place holder type representing a type by its name, to be
/// resolved later. This should be used only in situations where's its
/// guaranteed that the resolving will be done manually, there's no automatic
/// handling.
class TypeByName : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// id: The type name.
    ///
    /// l: Associated location.
    TypeByName(shared_ptr<ID> id, const Location& l=Location::None);

    /// Returns the referenced type's name.
    const shared_ptr<ID> id() const;

    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    shared_ptr<ID> _id;
};

/// A type representing an unset value.
class Unset : public PacType
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Unset(const Location& l=Location::None);

    ACCEPT_VISITOR(Type);
};

/// A type representing a member attribute of a composite type. The type can
/// be narrows to match only a specific attribute value.
class MemberAttribute : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// attr: If not given, the type matches any member attribute. If given,
    /// it matches only the specified ID value.
    MemberAttribute(shared_ptr<ID> attr = nullptr, const Location& l=Location::None);

    /// Returns the specific member attribute the type matches, or null if
    /// any.
    shared_ptr<ID> attribute() const;

    string render() override;
    bool equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    node_ptr<ID> _attribute;
};


/// A type representing a Module.
class Module : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Module(const Location& l=Location::None);

    ACCEPT_VISITOR(Type);
};

/// A type representing a non-existing return value.
class Void : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Void(const Location& l=Location::None);

    ACCEPT_VISITOR(Type);
};

/// Type for strings.
class String : public PacType
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    String(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for IP addresses.
class Address : public PacType, public trait::Parseable, public trait::Hashable {
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Address(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for IP subnets.
class Network : public PacType, public trait::Parseable, public trait::Hashable {
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Network(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for ports.
class Port : public PacType, public trait::Parseable, public trait::Hashable {
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Port(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for bitsets.
class Bitset : public PacType {
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

    /// Constructor creating a bitset type matching any other bitset type.
    Bitset(const Location& l=Location::None);

    // Returns the labels with their bit numbers.
    const label_list& labels() const;

    // Returns the bit number for a label, which must be known.
    int labelBit(shared_ptr<ID> label) const;

    shared_ptr<Scope> typeScope() override;
    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    label_list _labels;
    shared_ptr<Scope> _scope = nullptr;
};

/// Type for enums.
class Enum : public PacType {
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

    /// Constructor creating an enum type matching any other enum type.
    Enum(const Location& l=Location::None);

    // Returns the labels with their bit numbers.
    const label_list& labels() const;

    // Returns the value for a label, which must be known. Returns -1 for \c
    // Undef.
    int labelValue(shared_ptr<ID> label) const;

    shared_ptr<Scope> typeScope() override;
    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    label_list _labels;
    shared_ptr<Scope> _scope = nullptr;
};

/// Type for caddr values.
class CAddr : public PacType {
public:
    /// Constructor.
    ///
    /// l: Associated location.
    CAddr(const Location& l=Location::None);

    ACCEPT_VISITOR(Type);
};

/// Type for doubles.
class Double : public PacType, public trait::Parseable, public trait::Hashable
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Double(const Location& l=Location::None);

    ACCEPT_VISITOR(Type);
};

/// Type for booleans.
class Bool : public PacType, public trait::Parseable, public trait::Hashable
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Bool(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for interval values.
class Interval : public PacType, public trait::Parseable, public trait::Hashable
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Interval(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for time values.
class Time : public PacType, public trait::Parseable, public trait::Hashable
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Time(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};


/// Type for integer values.
class Integer : public PacType, public trait::Parameterized, public trait::Parseable, public trait::Hashable
{
public:
    /// Constructor.
    ///
    /// width: The bit width for the type.
    ///
    /// signed: True if it's a signed integer.
    ///
    /// l: Associated location.
    Integer(int width, bool signed_, const Location& l=Location::None);

    /// Constructore creating an integer type matching any other integer type.
    Integer(const Location& l=Location::None);

    /// Returns the types bit width.
    int width() const;

    /// Returns true if it's a signed integer type.
    bool signed_() const;

    bool _equal(shared_ptr<binpac::Type> other) const override;
    type_parameter_list parameters() const override;

    ACCEPT_VISITOR(PacType);

private:
    int _width;
    int _signed;
};

/// Type for tuples.
class Tuple : public PacType, public trait::Parameterized, public trait::TypeList {
public:
    /// Constructor.
    ///
    /// types: The types of the tuple's fields.
    ///
    /// l: Associated location.
    Tuple(const type_list& types, const Location& l=Location::None);

    /// Constructor for a wildcard tuple type matching any other.
    ///
    /// l: Associated location.
    Tuple(const Location& l=Location::None);

    const trait::TypeList::type_list typeList() const override;
    type_parameter_list parameters() const override;

    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    std::list<node_ptr<binpac::Type>> _types;
};

/// Type for types.
class TypeType : public binpac::Type
{
public:
    /// Constructor.
    ///
    /// type; The represented type.
    ///
    /// l: Associated location.
    TypeType(shared_ptr<binpac::Type> type, const Location& l=Location::None);

    /// Constructor for wildcard type.
    ///
    /// l: Associated location.
    TypeType(const Location& l=Location::None);

    shared_ptr<binpac::Type> typeType() const;

    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(binpac::Type);

private:
    node_ptr<binpac::Type> _rtype;
};

/// Type for exceptions.
class Exception : public TypedPacType
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

    /// Constructor. This creates a wildcard type matching any other exception type.
    Exception(const Location& l=Location::None);

    /// Returns the base exception type this one is derived from. Returns null
    /// for the top-level base exception.
    shared_ptr<Type> baseType() const;

    /// Sets the base exception type this one is derived from.
    void setBaseType(shared_ptr<Type> base);

    /// Returns true if this is the top-level root type.
    bool isRootType() const;

    /// Marks this type as defined by HILTI under the given name. Setting an
    /// empty name unmarks.
    void setLibraryType(const string& name);

    /// If the type is defined by HILTI, returns the name of the internal
    /// global. If not, returns an empty string.
    string libraryType() const;

    ACCEPT_VISITOR(TypedPacType);

private:
    node_ptr<Type> _base = nullptr;
    string _libtype;
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
    Parameter(shared_ptr<binpac::ID> id, shared_ptr<Type> type, bool constant, shared_ptr<Expression> default_value, Location l=Location::None);

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

/// Helper type to define a function parameter or return value.
class Result : public ast::type::mixin::function::Result<AstInfo>
{
public:
    /// Constructor for a return value.
    ///
    /// type: The type of the return value.
    ///
    /// constant: A flag indicating whether the return value is constant
    /// (i.e., a caller may not change its value.)
    ///
    /// l: A location associated with the expression.
    Result(shared_ptr<Type> type, bool constant, Location l=Location::None);

    ACCEPT_VISITOR_ROOT();
};

}

/// Type for functions.
class Function : public binpac::Type, public ast::type::mixin::Function<AstInfo>  {
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
    Function(shared_ptr<binpac::type::function::Result> result, const parameter_list& args, const Location& l=Location::None);

    /// Constructor for a function type that matches any other function type (i.e., a wildcard type).
    Function(const Location& l=Location::None);

    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(binpac::Type);
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
    Hook(shared_ptr<binpac::type::function::Result> result, const parameter_list& args, const Location& l=Location::None);

    /// Constructor for a hook type that matches any other hook type (i.e., a
    /// wildcard type).
    Hook(const Location& l=Location::None);

    ACCEPT_VISITOR(Function);
};

/// Type for bytes instances.
class Bytes : public PacType, public trait::Parseable, public trait::Hashable, public trait::Iterable
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Bytes(const Location& l=Location::None);

    shared_ptr<binpac::Type> iterType() override;
    shared_ptr<binpac::Type> elementType() override;
    shared_ptr<binpac::Type> fieldType() override;
    std::list<ParseAttribute> parseAttributes() const override;

    ACCEPT_VISITOR(PacType);
};

namespace iterator {

class Bytes : public Iterator
{
public:
    /// Constructor for an iterator over a bytes object.
    ///
    /// l: Associated location.
    ///
    Bytes(const Location& l=Location::None);

    ACCEPT_VISITOR(Iterator);
};

}
/// Type for file instances.
///
class File : public PacType
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    File(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

namespace iterator {

class ContainerIterator : public Iterator
{
protected:
    /// Constructor for an iterator over a container type.
    ///
    /// ctype: The container type this iterator iterates over.
    ///
    /// l: Associated location.
    ///
    ContainerIterator(shared_ptr<Type> ctype, const Location& l=Location::None);
};

class Map : public ContainerIterator
{
public:
    Map(shared_ptr<Type> ctype, const Location& l=Location::None);

    ACCEPT_VISITOR(Iterator);
};

class Set : public ContainerIterator
{
public:
    Set(shared_ptr<Type> ctype, const Location& l=Location::None);

    ACCEPT_VISITOR(Iterator);
};

class Vector : public ContainerIterator
{
public:
    Vector(shared_ptr<Type> ctype, const Location& l=Location::None);

    ACCEPT_VISITOR(Iterator);
};

class List : public ContainerIterator
{
public:
    List(shared_ptr<Type> ctype, const Location& l=Location::None);

    ACCEPT_VISITOR(Iterator);
};

}

/// Type for list objects.
class List : public TypedPacType, public trait::Container, public trait::Parseable
{
public:
    /// Constructor.
    ///
    /// etype: The type of the container's fields.
    ///
    /// l: Associated location.
    List(shared_ptr<Type> etype, const Location& l=Location::None);

    /// Constructor for a wildcard container type.
    List(const Location& l=Location::None);

    shared_ptr<binpac::Type> iterType() override;
    shared_ptr<binpac::Type> elementType() override;
    shared_ptr<binpac::Type> fieldType() override;

    ACCEPT_VISITOR(PacType);
};

// Type for vector objects.
class Vector : public TypedPacType, public trait::Container, public trait::Parseable
{
public:
    /// Constructor.
    ///
    /// etype: The type of the container's fields.
    ///
    /// l: Associated location.
    Vector(shared_ptr<Type> etype, const Location& l=Location::None);

    /// Constructor for a wildcard container type.
    Vector(const Location& l=Location::None);

    shared_ptr<binpac::Type> iterType() override;
    shared_ptr<binpac::Type> elementType() override;
    shared_ptr<binpac::Type> fieldType() override;

    ACCEPT_VISITOR(PacType);
};

/// Type for vector objects.
class Set : public TypedPacType, public trait::Container, public trait::Parseable
{
public:
    /// Constructor.
    ///
    /// etype: The type of the container's fields.
    ///
    /// l: Associated location.
    Set(shared_ptr<Type> etype, const Location& l=Location::None);

    /// Constructor for a wildcard container type.
    Set(const Location& l=Location::None);

    shared_ptr<binpac::Type> iterType() override;
    shared_ptr<binpac::Type> elementType() override;
    shared_ptr<binpac::Type> fieldType() override;

    ACCEPT_VISITOR(PacType);
};

/// Type for map objects.
class Map : public PacType, public trait::Parameterized, public trait::Container
{
public:
    /// Constructor.
    ///
    /// key: The type of the container's index.
    ///
    /// value: The type of the container's values.
    ///
    /// l: Associated location.
    Map(shared_ptr<binpac::Type> key, shared_ptr<binpac::Type> value, const Location& l=Location::None);

    /// Constructor for a wildcard map type.
    Map(const Location& l=Location::None) : PacType(l) { setWildcard(true); }

    shared_ptr<binpac::Type> keyType() const { return _key; }
    shared_ptr<binpac::Type> valueType() const { return _value; }

    type_parameter_list parameters() const override;
    shared_ptr<binpac::Type> iterType() override;
    shared_ptr<binpac::Type> elementType() override;
    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    node_ptr<binpac::Type> _key = nullptr;
    node_ptr<binpac::Type> _value = nullptr;
};

/// Type for regexp instances.
class RegExp : public PacType, public trait::Parameterized, public trait::Parseable
{
public:
    /// Constructor.
    ///
    /// attrs: List of optional attributes controlling specifics of the
    /// regular expression. These must include the ampersand, e.g., \c &nosub.
    ///
    /// l: Associated location.
    RegExp(const attribute_list& attrs, const Location& l=Location::None);

    /// Constructore creating an IOSrc type matching any other (i.e., \c regexp<*>).
    RegExp(const Location& l=Location::None);

    /// Returns the enum label for the type of RegExp.
    shared_ptr<AttributeSet> attributes() const;

    bool _equal(shared_ptr<binpac::Type> other) const override;
    type_parameter_list parameters() const override;

    ACCEPT_VISITOR(Type);

private:
    shared_ptr<AttributeSet> _attrs;
};

/// Type for a timer_mgr object.
///
class TimerMgr : public PacType
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    TimerMgr(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for a timer object.
///
class Timer : public PacType
{
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Timer(const Location& l=Location::None);

    ACCEPT_VISITOR(PacType);
};

namespace unit {

/// Base type for a unit items.
class Item : public Node
{
public:
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// type: The type of the item.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// l: Location associated with the item.
    Item(shared_ptr<ID> id,
         const hook_list& hooks = hook_list(),
         const attribute_list& attrs = attribute_list(),
         const Location& l=Location::None);

    /// Returns the item's identifier.
    shared_ptr<ID> id() const;

    /// Returns the item's scope. The scope may define identifier's local to
    /// expressions and hooks associated with the item. This method will
    /// return null until the ScopeBuilder has run.
    shared_ptr<Scope> scope() const;

    /// Returns the hooks associated with this item.
    hook_list hooks() const;

    /// Returns the attributes associated with the item.
    shared_ptr<AttributeSet> attributes() const;

    ACCEPT_VISITOR_ROOT();

private:
    node_ptr<ID> _id;
    node_ptr<AttributeSet> _attrs;
    std::list<node_ptr<Hook>> _hooks;

    shared_ptr<Scope> _scope = nullptr;
};

namespace item {

/// Base class for unit fields parsed from the input.
class Field : public Item
{
public:
    /// Returns the item's type.
    shared_ptr<binpac::Type> type() const;

    /// Returns the item's associated default value, or null if none.
    shared_ptr<Expression> default_() const;

    /// Returns the item's associated condition, or null if none.
    shared_ptr<Expression> condition() const;

    /// Returns the list of attached sinks.
    expression_list sinks() const;

    ACCEPT_VISITOR(Item);

protected:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// type: The type of the item.
    ///
    /// default: An optional default value asociated with the item.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    Field(shared_ptr<ID> id,
         shared_ptr<binpac::Type> type,
         shared_ptr<Expression> value = nullptr,
         shared_ptr<Expression> cond = nullptr,
         const hook_list& hooks = hook_list(),
         const attribute_list& attrs = attribute_list(),
         const expression_list& sinks = expression_list(),
         const Location& l=Location::None);

private:
    shared_ptr<binpac::Type> _type;
    node_ptr<Expression> _default;
    node_ptr<Expression> _cond;
    expression_list _sinks;
};

namespace field {

/// A constant unit field.
class Constant : public Field
{
public:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// val: The constant's value.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// l: Location associated with the item.
    Constant(shared_ptr<Expression> value = nullptr,
             shared_ptr<Expression> cond = nullptr,
             const hook_list& hooks = hook_list(),
             const Location& l=Location::None);

    ACCEPT_VISITOR(Field);
};

/// A unit field based on its type.
class Type : public Field
{
public:
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// default_: The field's default, or null if none.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// params: List of parameters passed to sub-type's parsing. Only relevant
    /// if \a type is Unit type.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    Type(shared_ptr<ID> id,
         shared_ptr<binpac::Type> type,
         shared_ptr<Expression> default_ = nullptr,
         shared_ptr<Expression> cond = nullptr,
         const hook_list& hooks = hook_list(),
         const attribute_list& attrs = attribute_list(),
         const expression_list& params = expression_list(),
         const expression_list& sinks = expression_list(),
         const Location& l=Location::None);

    /// Returns the parameters passed to sub-type's parsing.
    expression_list parameters() const;

    ACCEPT_VISITOR(Field);

private:
    std::list<node_ptr<Expression>> _params;
};

/// A unit field defined by a regular expression.
class RegExp : public Field
{
public:
    /// Constructor.
    ///
    /// regex: The regular expression for parsing the field.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// default_: The field's default, or null if none.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    RegExp(const string& regexp,
           shared_ptr<ID> id,
           shared_ptr<Expression> default_,
           shared_ptr<Expression> cond = nullptr,
           const hook_list& hooks = hook_list(),
           const attribute_list& attrs = attribute_list(),
           const expression_list& sinks = expression_list(),
           const Location& l=Location::None);

    /// Returns the regular expression.
    const string& regexp() const;

    ACCEPT_VISITOR(Field);

private:
    string _regexp;
};

namespace switch_ {

class Case : public Node
{
public:
    /// Constructor.
    ///
    /// exprs: Expression associated with the case. Empty to mark the default case.
    ///
    /// item: The item implementing the case.
    ///
    /// l: Location associated with the case.
    Case(const expression_list& exprs, shared_ptr<Item> item, const Location& l=Location::None);

    /// Returns the case's expression.
    expression_list expressions() const;

    /// Returns the case's implementation item.
    shared_ptr<Item> item() const;

    ACCEPT_VISITOR_ROOT();

private:
    std::list<node_ptr<Expression>> _exprs;
    shared_ptr<Item> _item;
};

}

/// Type for a unit switch item
class Switch : public Field
{
public:
    typedef std::list<shared_ptr<switch_::Case>> case_list;

    /// Constructor.
    ///
    /// expr: The switch's expression.
    ///
    /// cases: The list of switch cases.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// l: Location associated with the item.
    Switch(shared_ptr<Expression> expr, const case_list& cases, const hook_list& hooks = hook_list(), const Location& l=Location::None);

    /// Returns the switch's expression.
    shared_ptr<Expression> expression() const;

    /// Returns the list of cases.
    case_list cases() const;

    ACCEPT_VISITOR(Item);

private:
    node_ptr<Expression> _expr;
    std::list<node_ptr<switch_::Case>> _cases;
};

}

/// A user-defined unit variable.
class Variable : public Item
{
public:
    /// Constructor.
    ///
    /// id: The name of declared property. If it does not have a leading dot,
    /// that will be added.
    ///
    /// type: The variable's type.
    ///
    /// default_: An optional default value.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// l: An associated location.
    Variable(shared_ptr<binpac::ID> id,
             shared_ptr<binpac::Type> type,
             shared_ptr<Expression> default_,
             const hook_list& hooks = hook_list(),
             const Location& l=Location::None);

    /// Returns the variable's default, or null if none.
    shared_ptr<Expression> default_() const;

    /// Returns the variable's type.
    shared_ptr<binpac::Type> type() const;

    ACCEPT_VISITOR(Item);

private:
    node_ptr<Expression> _default;
    node_ptr<binpac::Type> _type;
};

/// A unit property.
class Property : public Item
{
public:
    /// Constructor.
    ///
    /// id: The name of declared property. If it does not have a leading dot,
    /// that will be added.
    ///
    /// value: The properties value.
    ///
    /// l: An associated location.
    Property(shared_ptr<binpac::ID> id, shared_ptr<binpac::Expression> value, const Location& l=Location::None);

    /// Returns the properties value.
    shared_ptr<binpac::Expression> value() const;

    ACCEPT_VISITOR(Item);

private:
    node_ptr<binpac::Expression> _value;
};

/// A unit-wide hook.
class GlobalHook : public Item
{
public:
    /// Constructor.
    ///
    /// id: The name of the hook.
    ///
    /// hook: The hook.
    ///
    /// l: An associated location.
    GlobalHook(shared_ptr<ID> id, shared_ptr<binpac::Hook> hook, const Location& l=Location::None);

    /// Returns the hook.
    shared_ptr<binpac::Hook> hook() const;

    ACCEPT_VISITOR(Item);

private:
    node_ptr<ID> _id;
    node_ptr<binpac::Hook> _hook;
};

}

}

/// Type for units.
class Unit : public PacType, public trait::Parseable
{
public:
    typedef std::list<shared_ptr<unit::item::Field>> field_list;

    /// Constructor.
    ///
    /// items: The unit's items.
    ///
    /// l: Associated location.
    Unit(const parameter_list& params, const unit_item_list& items, const Location& l=Location::None);

    /// Constructor for a wildcard unit type matching any other.
    ///
    /// l: Associated location.
    Unit(const Location& l=Location::None);

    /// Returns the type's parameters.
    parameter_list parameters() const;

    /// Returns the list of unit items.
    unit_item_list items() const;

    /// Returns a list of all fields. This is a convinience method that
    /// prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::Field>> fields() const;

    /// Returns a list of all variables. This is a convinience method that
    /// prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::Variable>> variables() const;

    /// Returns a list of all global-wide hooks. This is a convinience method
    /// that prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::GlobalHook>> hooks() const;

    /// Returns a list of all properties. This is a convinience method that
    /// prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::Property>> properties() const;

    /// Returns the item of a given name, or null if there's no such item.
    ///
    /// id: The item to look up.
    shared_ptr<unit::Item> item(shared_ptr<ID> id) const;

    /// Returns the unit's scope. The scope may define identifier's local to
    /// expressions and hooks associated with the unit or any of its items.
    /// This method will return null until the ScopeBuilder has run.
    shared_ptr<Scope> scope() const;

    /// Returns the grammar for this type, if set. Intially, this is unset
    /// but will be initialized by the grammar builder.
    shared_ptr<Grammar> grammar() const;

    bool _equal(shared_ptr<binpac::Type> other) const override;

    ACCEPT_VISITOR(Type);

protected:
    friend class passes::GrammarBuilder;

    void setGrammar(shared_ptr<Grammar> grammar);

private:
    std::list<node_ptr<type::function::Parameter>> _params;
    std::list<node_ptr<unit::Item>> _items;

    shared_ptr<Scope> _scope = nullptr;
    shared_ptr<Grammar> _grammar = nullptr;
};

////

}

}

#endif

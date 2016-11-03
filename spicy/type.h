
#ifndef SPICY_TYPE_H
#define SPICY_TYPE_H

#include <ast/type.h>

#include "common.h"
#include "id.h"

namespace spicy {

namespace passes {
class GrammarBuilder;
}

class AttributeSet;
class Grammar;

namespace type {

namespace trait {

// Base class for type trait. We use trait to mark types that have certain properties.
//
// Note that Traits aren't (and can't) be derived from Node and thus aren't AST nodes.
class Trait : virtual public ast::rtti::Base {
    AST_RTTI
public:
    Trait();
    virtual ~Trait();
};

namespace parameter {

/// Base class for a type parameter. This is used with trait::Parameterized.
class Base : virtual public ast::rtti::Base {
    AST_RTTI
public:
    virtual ~Base();

    /// Will be called to compare this node against another of the *same*
    /// type.
    virtual bool _equal(shared_ptr<Base> other) const = 0;
};

/// A type parameter representing a Type.
class Type : public Base {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// type: The type the parameter specifies.
    Type(shared_ptr<spicy::Type> type);

    /// Returns the type the parameter specifies.
    shared_ptr<spicy::Type> type() const;

    bool _equal(shared_ptr<Base> other) const override;

private:
    node_ptr<spicy::Type> _type;
};

/// A type parameter representing an integer value.
class Integer : public Base {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// value: The parameter's integer value.
    Integer(int64_t value);

    /// Returns the parameters integer value.
    int64_t value() const;

    bool _equal(shared_ptr<Base> other) const override;

private:
    int64_t _value;
};

/// A type parameter representing an enum value. This must include the scope
/// for a module-level lookup.
class Enum : public Base {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// label: The parameter's enum identifier.
    Enum(shared_ptr<ID> label);

    /// Returns the parameters enum identifier.
    shared_ptr<ID> label() const;

    bool _equal(shared_ptr<Base> other) const override;

private:
    shared_ptr<ID> _label;
};

class Attribute : public Base {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// value: The parameter's attribute, including the ampersand.
    Attribute(const string& attr);

    /// Returns the attribute.
    const string& value() const;

    bool _equal(shared_ptr<Base> other) const override;

private:
    string _attr;
};
}

/// Trait class marking a type with parameters.
class Parameterized : public virtual Trait {
    AST_RTTI
public:
    typedef std::list<shared_ptr<parameter::Base>> parameter_list;

    /// Compares the type with another for equivalence, which must be an
    /// instance of the same type class.
    bool equal(shared_ptr<spicy::Type> other) const;

    /// Returns the type's parameters. Note we return this by value as the
    /// derived classes need to build the list on the fly each time so that
    /// potential updates to the members (in particular type resolving) get
    /// reflected.
    virtual parameter_list parameters() const = 0;
};

/// Trait class marking a composite type that has a series of subtypes.
class TypeList : public virtual Trait {
    AST_RTTI
public:
    typedef std::list<shared_ptr<spicy::Type>> type_list;

    /// Returns the ordered list of subtypes.
    virtual const type_list typeList() const = 0;
};

/// Trait class marking a type that provides iterators.
class Iterable : public virtual Trait {
    AST_RTTI
public:
    /// Returns the type for an iterator over this type.
    virtual shared_ptr<spicy::Type> iterType() = 0;

    /// Returns the type of elements when iterating over this type.
    virtual shared_ptr<spicy::Type> elementType() = 0;
};

/// Trait class marking a container type.
class Container : public Iterable {
    AST_RTTI
};

/// Trait class marking a type that can be hashed for storing in containers.
class Hashable : public virtual trait::Trait {
    AST_RTTI
};

/// Trait class marking a type that can be parsed from an input stream. Only
/// these types can be used inside a ~~Unit.
class Parseable : public virtual trait::Trait {
    AST_RTTI
public:
    virtual ~Parseable();

    /// Returns the type of values directly parsed by this type. This type
    /// will become the type of the corresponding Unit item. While usually
    /// the parsed type is the same as the type itself (and that's what the
    /// default implementation returns), it does not need to. The RegExp type
    /// returns Bytes for example.
    ///
    /// The default implementation returns just \a this.
    ///
    /// Returns: The type for parsed items.
    virtual shared_ptr<spicy::Type> fieldType();

    /// A description of a custom type attribute, as returned by
    /// parseAttributes*().
    struct ParseAttribute {
        string key;            /// The attribute name.
        shared_ptr<Type> type; /// The attribute's expression type, or null for no argument.
        shared_ptr<Expression> default_; /// A default value for the expression if none is given, or
                                         /// null for no default.
        bool implicit_; /// If true and *default* is given, the attribute is implicitly assumed to
                        /// be present even if not given.
    };

    /// Returns the custom attributes the type supports for parsing. Note
    /// that these are added on top of any attributes supported generally by
    /// all types (e.g., \a &default).
    ///
    /// The default implementation returns the empty list.
    virtual std::list<ParseAttribute> parseAttributes() const;
};


/// Trait class marking a type to which a Sink can directly be attached.
class Sinkable : public virtual trait::Trait {
    AST_RTTI
};
}
}

/// Base class for all AST nodes representing a type.
class Type : public ast::Type<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Type(const Location& l = Location::None);

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
class OptionalArgument : public spicy::Type {
    AST_RTTI
public:
    OptionalArgument(shared_ptr<Type> arg);

    /// Returns the argument, or null if not given.
    shared_ptr<spicy::Type> argType() const;

    string render() override;
    bool equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    shared_ptr<Type> _arg;
};

/// Base class for instantiable Spicy types.
class PacType : public spicy::Type {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    PacType(const Location& l = Location::None);

    /// Constructor.
    ///
    /// attrs: Attributes associated with the type.
    ///
    /// l: Associated location.
    PacType(const attribute_list& attrs, const Location& l = Location::None);

    /// Returns the attributes associated with the type.
    shared_ptr<AttributeSet> attributes() const;

    ACCEPT_VISITOR(Type);

protected:
    node_ptr<AttributeSet> _attrs;
};

/// Base class for instatiable types parameterized with a single type parameter.
class TypedPacType : public PacType, public trait::Parameterized {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// argtype: The type parameter.
    ///
    /// l: Associated location.
    TypedPacType(shared_ptr<Type> argtype, const Location& l = Location::None);

    /// Constructor for a wildcard type.
    ///
    /// l: Associated location.
    TypedPacType(const Location& l = Location::None);

    /// Returns the type's parameter.
    shared_ptr<Type> argType() const;

    parameter_list parameters() const override;
    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    node_ptr<Type> _argtype;
};

/// Base type for iterators.
class Iterator : public TypedPacType {
    AST_RTTI
public:
    /// Constructor for a wildcard iterator type.
    ///
    /// l: Associated location.
    Iterator(const Location& l = Location::None);

    /// Constructor for an iterator over a given target tyoe.
    ///
    /// ttype: The target type. This must be a trait::Iterable.
    ///
    /// l: Associated location.
    Iterator(shared_ptr<Type> ttype, const Location& l = Location::None);

    bool equal(shared_ptr<Type> other) const override;

    ACCEPT_VISITOR(TypedPacType);

private:
    node_ptr<Type> _ttype;
};

/// A tupe matching any other type.
class Any : public spicy::Type {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Any(const Location& l = Location::None);

    ACCEPT_VISITOR(Type);
};

/// A place holder type representing a type that has not been resolved yet.
class Unknown : public spicy::Type {
    AST_RTTI
public:
    /// Constructor for a type not further specified. This must be resolved by
    /// some external means.
    ///
    /// l: Associated location.
    Unknown(const Location& l = Location::None);

    /// Constructor referencing a type by its name. This will be resolved
    /// automatically by resolver.
    Unknown(shared_ptr<ID> id, const Location& l = Location::None);

    /// If an ID is associated with the type, returns it; null otherwise.
    shared_ptr<ID> id() const;

    ACCEPT_VISITOR(Type);

private:
    node_ptr<ID> _id = nullptr;
};

/// A place-holder for the element type of a iterable that we may not have
/// resolved yet. It will later be replaced with the actual element type.
class UnknownElementType : public spicy::Type {
    AST_RTTI
public:
    /// Constructor referencing an expression that evaluate to a iterable.
    /// The iterable's element type will eventually substitute for this
    /// type.
    ///
    /// Note that we take a node_ptr here to track any changes.
    UnknownElementType(node_ptr<Expression> expr, const Location& l = Location::None);

    /// Returns the iterable expression associated with the type.
    shared_ptr<Expression> expression() const;

    ACCEPT_VISITOR(Type);

private:
    node_ptr<Expression> _expr = nullptr;
};

/// An internal place holder type representing a type by its name, to be
/// resolved later. This should be used only in situations where's its
/// guaranteed that the resolving will be done manually, there's no automatic
/// handling.
class TypeByName : public spicy::Type {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The type name.
    ///
    /// l: Associated location.
    TypeByName(shared_ptr<ID> id, const Location& l = Location::None);

    /// Constructor.
    ///
    /// id: The type name.
    ///
    /// l: Associated location.
    TypeByName(const string& id, const Location& l = Location::None);

    /// Returns the referenced type's name.
    const shared_ptr<ID> id() const;

    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    shared_ptr<ID> _id;
};

/// A type representing an unset value.
class Unset : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Unset(const Location& l = Location::None);

    ACCEPT_VISITOR(Type);
};

/// A type representing a member attribute of a composite type. The type can
/// be narrows to match only a specific attribute value.
class MemberAttribute : public spicy::Type {
    AST_RTTI
public:
    /// Constructor for a type that matches only a specific attribute.
    ///
    /// attr: The attribute value to match.
    MemberAttribute(shared_ptr<ID> attr, const Location& l = Location::None);

    /// Constructor for a wildcard map type.
    MemberAttribute(const Location& l = Location::None);

    /// Returns the specific member attribute the type matches, or null if
    /// any.
    shared_ptr<ID> attribute() const;

    string render() override;
    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    node_ptr<ID> _attribute;
};


/// A type representing a Module.
class Module : public spicy::Type {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Module(const Location& l = Location::None);

    ACCEPT_VISITOR(Type);
};

/// A type representing a non-existing return value.
class Void : public spicy::Type, public trait::Parseable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Void(const Location& l = Location::None);

    ACCEPT_VISITOR(Type);
};

/// Type for strings.
class String : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    String(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for IP addresses.
class Address : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Address(const Location& l = Location::None);

    std::list<ParseAttribute> parseAttributes() const override;

    ACCEPT_VISITOR(PacType);
};

/// Type for IP subnets.
class Network : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Network(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for ports.
class Port : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Port(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for bitsets.
class Bitset : public PacType {
    AST_RTTI
public:
    typedef std::pair<shared_ptr<ID>, int> Label;
    typedef std::list<Label> label_list;

    /// Constructor.
    ///
    /// labels: List of pairs of label name and bit number. If a bit number is
    /// -1, it will be chosen automaticallty.
    ///
    /// l: Associated location.
    Bitset(const label_list& labels, const Location& l = Location::None);

    /// Constructor creating a bitset type matching any other bitset type.
    Bitset(const Location& l = Location::None);

    // Returns the labels with their bit numbers.
    const label_list& labels() const;

    // Returns the bit number for a label, which must be known.
    int labelBit(shared_ptr<ID> label) const;

    shared_ptr<Scope> typeScope() override;
    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    label_list _labels;
    shared_ptr<Scope> _scope = nullptr;
};

namespace bitfield {

/// Class describing one element of an integer's bitfield.
class Bits : public Node {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name associated with the bits.
    ///
    /// lower: The lower bit of the range.
    ///
    /// upper: The upper bit of the range (inclusive).
    ///
    /// attrs: Optional type attributes associated with the bits.
    ///
    /// l: Associated location.
    Bits(shared_ptr<ID> id, int lower, int upper, int parent_width,
         const attribute_list& attrs = attribute_list(), const Location& l = Location::None);

    /// Returns the name associated with the bits.
    shared_ptr<ID> id() const;

    /// Returns the lower bit of the range.
    int lower() const;

    /// Returns the upper bit of the range (inclusive).
    int upper() const;

    /// Returns the attributes associated with the bits.
    shared_ptr<AttributeSet> attributes() const;

    /// Returns the type final type with which this bit range is to be
    /// stored. This takes into account a potential &convert attribute.
    shared_ptr<Type> fieldType() const;

    ACCEPT_VISITOR_ROOT();

private:
    node_ptr<ID> _id;
    node_ptr<AttributeSet> _attrs;
    int _lower;
    int _upper;
    int _parent_width;
};
}

/// Type for bitfields. Bitfields have an underlying integer value of a given
/// size and extract a set of bitranges from it, identified by an ID for each name,
/// Type for integer values.
class Bitfield : public PacType,
                 public trait::Parameterized,
                 public trait::Parseable,
                 public trait::Hashable {
    AST_RTTI
public:
    typedef std::list<shared_ptr<bitfield::Bits>> bits_list;

    /// Constructor.
    ///
    /// width: The bit width for the underlying integer.
    ///
    /// l: Associated location.
    Bitfield(int width, const bits_list& bits, const Location& l = Location::None);

    /// Constructore creating an integer type matching any other integer type.
    Bitfield(const Location& l = Location::None);

    /// Returns the types bit width.
    int width() const;

    /// Associated names with subsets of bits.
    void setBits(const bits_list& bits);

    /// Returns the bit order for interpreting bit ranges. This returns an
    /// expression of type Spicy::BitOrder.
    shared_ptr<Expression> bitOrder();

    /// Sets the bit ordering used to interpret bit ranges. Default is LSB0.
    ///
    /// order: The order to set. The must be an expression of type type
    /// Spicy::BitOrder..
    void setBitOrder(shared_ptr<Expression> order);

    /// Returns the names associated with subsets of bits.
    bits_list bits() const;

    /// Returns the bits associated with a given name, or null if no such name.
    shared_ptr<bitfield::Bits> bits(shared_ptr<ID> id) const;

    parameter_list parameters() const override;
    std::list<ParseAttribute> parseAttributes() const override;

    ACCEPT_VISITOR(PacType);

private:
    std::list<node_ptr<bitfield::Bits>> _bits;
    node_ptr<Expression> _bit_order;
    int _width;
};

/// Type for enums.
class Enum : public PacType {
    AST_RTTI
public:
    typedef std::pair<shared_ptr<ID>, int> Label;
    typedef std::list<Label> label_list;

    /// Constructor.
    ///
    /// labels: List of pairs of label name and value. If a bit number is -1,
    /// it will be chosen automaticallty.
    ///
    /// l: Associated location.
    Enum(const label_list& labels, const Location& l = Location::None);

    /// Constructor creating an enum type matching any other enum type.
    Enum(const Location& l = Location::None);

    // Returns the labels with their bit numbers.
    const label_list& labels() const;

    // Returns the value for a label, which must be known. Returns -1 for \c
    // Undef.
    int labelValue(shared_ptr<ID> label) const;

    shared_ptr<Scope> typeScope() override;
    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    label_list _labels;
    shared_ptr<Scope> _scope = nullptr;
};

/// Type for caddr values.
class CAddr : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    CAddr(const Location& l = Location::None);

    ACCEPT_VISITOR(Type);
};

/// Type for doubles.
class Double : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Double(const Location& l = Location::None);

    std::list<ParseAttribute> parseAttributes() const override;

    ACCEPT_VISITOR(Type);
};

/// Type for booleans.
class Bool : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Bool(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for interval values.
class Interval : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Interval(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for time values.
class Time : public PacType, public trait::Parseable, public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Time(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for integer values.
class Integer : public PacType,
                public trait::Parameterized,
                public trait::Parseable,
                public trait::Hashable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// width: The bit width for the type.
    ///
    /// signed: True if it's a signed integer.
    ///
    /// l: Associated location.
    Integer(int width, bool signed_, const Location& l = Location::None);

    /// Constructore creating an integer type matching any other integer type.
    Integer(const Location& l = Location::None);

    /// Returns the types bit width.
    int width() const;

    /// Returns true if it's a signed integer type.
    bool signed_() const;

    bool _equal(shared_ptr<spicy::Type> other) const override;
    parameter_list parameters() const override;
    std::list<ParseAttribute> parseAttributes() const override;

    /// XXX
    static shared_ptr<Integer> signedInteger(int width, const Location& l = Location::None);

    /// XXX
    static shared_ptr<Integer> unsignedInteger(int width, const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);

private:
    int _width;
    int _signed;
};

/// Type for tuples.
class Tuple : public PacType, public trait::Parameterized, public trait::TypeList {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// types: The types of the tuple's fields.
    ///
    /// l: Associated location.
    Tuple(const type_list& types, const Location& l = Location::None);

    /// Constructor for a wildcard tuple type matching any other.
    ///
    /// l: Associated location.
    Tuple(const Location& l = Location::None);

    const trait::TypeList::type_list typeList() const override;
    parameter_list parameters() const override;

    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(Type);

private:
    std::list<node_ptr<spicy::Type>> _types;
};

/// Type for types.
class TypeType : public spicy::Type {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// type; The represented type.
    ///
    /// l: Associated location.
    TypeType(shared_ptr<spicy::Type> type, const Location& l = Location::None);

    /// Constructor for wildcard type.
    ///
    /// l: Associated location.
    TypeType(const Location& l = Location::None);

    shared_ptr<spicy::Type> typeType() const;

    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(spicy::Type);

private:
    node_ptr<spicy::Type> _rtype;
};

/// Type for exceptions.
class Exception : public TypedPacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// base: Another exception type this one derived from. If null, the
    /// top-level base exception is used.
    ///
    /// arg: The type of the exceptions argument, or null if none.
    ///
    /// l: Location associated with the type.
    Exception(shared_ptr<Type> base, shared_ptr<Type> arg, const Location& l = Location::None);

    /// Constructor. This creates a wildcard type matching any other exception type.
    Exception(const Location& l = Location::None);

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
class Parameter : public ast::type::mixin::function::Parameter<AstInfo> {
    AST_RTTI
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
    /// clear: A flag indicating whether the parameter has the \c __clear attribute.
    ///
    /// default_value: An optional default value for the parameters, or null if none.
    ///
    /// l: A location associated with the expression.
    Parameter(shared_ptr<spicy::ID> id, shared_ptr<Type> type, bool constant, bool clear,
              shared_ptr<Expression> default_value, Location l = Location::None);

    /// Constructor for a return value.
    ///
    /// type: The type of the return value.
    ///
    /// constant: A flag indicating whether the return value is constant
    /// (i.e., a caller may not change its value.)
    ///
    /// l: A location associated with the expression.
    Parameter(shared_ptr<Type> type, bool constant, Location l = Location::None);

    /// Returns true if the parameter has the \c clear attribute set.
    bool clear() const;

    ACCEPT_VISITOR_ROOT();

private:
    bool _clear;
};

/// Helper type to define a function parameter or return value.
class Result : public ast::type::mixin::function::Result<AstInfo> {
    AST_RTTI
public:
    /// Constructor for a return value.
    ///
    /// type: The type of the return value.
    ///
    /// constant: A flag indicating whether the return value is constant
    /// (i.e., a caller may not change its value.)
    ///
    /// l: A location associated with the expression.
    Result(shared_ptr<Type> type, bool constant, Location l = Location::None);

    ACCEPT_VISITOR_ROOT();
};

/// Spicy's supported calling conventions.
enum CallingConvention {
    SPICY,         /// Default calling convention for Spicy functions.
    SPICY_HILTI,   /// A HILTI function with additional implicit Spicy parameters.
    HILTI,          /// A HILTI function.
    SPICY_HILTI_C, /// A function with HILTI-C calling convention and additional implicit Spicy
                    /// parameters.
    HILTI_C,        /// A function with HILTI-C calling convention.
    C               /// A function with standard C calling convention.
};
}

/// Type for functions.
class Function : public spicy::Type, public ast::type::mixin::Function<AstInfo> {
    AST_RTTI
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
    Function(shared_ptr<spicy::type::function::Result> result, const parameter_list& args,
             function::CallingConvention cc = function::SPICY, const Location& l = Location::None);

    /// Constructor for a function type that matches any other function type (i.e., a wildcard
    /// type).
    Function(const Location& l = Location::None);

    /// Returns the function's calling convention.
    function::CallingConvention callingConvention() const;

    /// Sets the function's calling convention.
    void setCallingConvention(function::CallingConvention cc);

    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(spicy::Type);

private:
    function::CallingConvention _cc;
};

/// Type for hooks.
class Hook : public Function {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// result: The hook's return type.
    ///
    /// params: The hooks's parameters.
    ///
    /// l: Associated location.
    Hook(shared_ptr<spicy::type::function::Result> result, const parameter_list& args,
         const Location& l = Location::None);

    /// Constructor for a hook type that matches any other hook type (i.e., a
    /// wildcard type).
    Hook(const Location& l = Location::None);

    ACCEPT_VISITOR(Function);
};

/// Type for bytes instances.
class Bytes : public PacType,
              public trait::Parseable,
              public trait::Hashable,
              public trait::Iterable,
              public trait::Sinkable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Bytes(const Location& l = Location::None);

    shared_ptr<spicy::Type> iterType() override;
    shared_ptr<spicy::Type> elementType() override;
    std::list<ParseAttribute> parseAttributes() const override;

    ACCEPT_VISITOR(PacType);
};

namespace iterator {

class Bytes : public Iterator {
    AST_RTTI
public:
    /// Constructor for an iterator over a bytes object.
    ///
    /// l: Associated location.
    ///
    Bytes(const Location& l = Location::None);

    ACCEPT_VISITOR(Iterator);
};
}
/// Type for file instances.
///
class File : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    File(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

namespace iterator {

class ContainerIterator : public Iterator {
protected:
    /// Constructor for an iterator over a container type.
    ///
    /// ctype: The container type this iterator iterates over.
    ///
    /// l: Associated location.
    ///
    ContainerIterator(shared_ptr<Type> ctype, const Location& l = Location::None);
};

class Map : public ContainerIterator {
    AST_RTTI
public:
    Map(shared_ptr<Type> ctype, const Location& l = Location::None);

    ACCEPT_VISITOR(Iterator);
};

class Set : public ContainerIterator {
    AST_RTTI
public:
    Set(shared_ptr<Type> ctype, const Location& l = Location::None);

    ACCEPT_VISITOR(Iterator);
};

class Vector : public ContainerIterator {
    AST_RTTI
public:
    Vector(shared_ptr<Type> ctype, const Location& l = Location::None);

    ACCEPT_VISITOR(Iterator);
};

class List : public ContainerIterator {
    AST_RTTI
public:
    List(shared_ptr<Type> ctype, const Location& l = Location::None);

    ACCEPT_VISITOR(Iterator);
};
}

/// Type for list objects.
class List : public TypedPacType, public trait::Parseable, public trait::Container {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// etype: The type of the container's fields.
    ///
    /// l: Associated location.
    List(shared_ptr<Type> etype, const Location& l = Location::None);

    /// Constructor for a wildcard container type.
    List(const Location& l = Location::None);

    shared_ptr<spicy::Type> iterType() override;
    shared_ptr<spicy::Type> elementType() override;
    std::list<ParseAttribute> parseAttributes() const override;

    ACCEPT_VISITOR(PacType);
};

// Type for vector objects.
class Vector : public TypedPacType, public trait::Container, public trait::Parseable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// etype: The type of the container's fields.
    ///
    /// l: Associated location.
    Vector(shared_ptr<Type> etype, const Location& l = Location::None);

    /// Constructor for a wildcard container type.
    Vector(const Location& l = Location::None);

    shared_ptr<spicy::Type> iterType() override;
    shared_ptr<spicy::Type> elementType() override;

    ACCEPT_VISITOR(PacType);
};

/// Type for vector objects.
class Set : public TypedPacType, public trait::Container, public trait::Parseable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// etype: The type of the container's fields.
    ///
    /// l: Associated location.
    Set(shared_ptr<Type> etype, const Location& l = Location::None);

    /// Constructor for a wildcard container type.
    Set(const Location& l = Location::None);

    shared_ptr<spicy::Type> iterType() override;
    shared_ptr<spicy::Type> elementType() override;

    ACCEPT_VISITOR(PacType);
};

/// Type for map objects.
class Map : public PacType, public trait::Parameterized, public trait::Container {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// key: The type of the container's index.
    ///
    /// value: The type of the container's values.
    ///
    /// l: Associated location.
    Map(shared_ptr<spicy::Type> key, shared_ptr<spicy::Type> value,
        const Location& l = Location::None);

    /// Constructor for a wildcard map type.
    Map(const Location& l = Location::None) : PacType(l)
    {
        setWildcard(true);
    }

    shared_ptr<spicy::Type> keyType() const
    {
        return _key;
    }
    shared_ptr<spicy::Type> valueType() const
    {
        return _value;
    }

    parameter_list parameters() const override;
    shared_ptr<spicy::Type> iterType() override;
    shared_ptr<spicy::Type> elementType() override;
    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(PacType);

private:
    node_ptr<spicy::Type> _key = nullptr;
    node_ptr<spicy::Type> _value = nullptr;
};

/// Type for regexp instances.
class RegExp : public PacType, public trait::Parameterized, public trait::Parseable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// attrs: List of optional attributes controlling specifics of the
    /// regular expression. These must include the ampersand, e.g., \c &nosub.
    ///
    /// l: Associated location.
    RegExp(const attribute_list& attrs, const Location& l = Location::None);

    /// Constructore creating an IOSrc type matching any other (i.e., \c regexp<*>).
    RegExp(const Location& l = Location::None);

    bool _equal(shared_ptr<spicy::Type> other) const override;
    parameter_list parameters() const override;
    shared_ptr<spicy::Type> fieldType() override;

    ACCEPT_VISITOR(Type);

private:
    shared_ptr<AttributeSet> _attrs;
};

/// Type for a timer_mgr object.
///
class TimerMgr : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    TimerMgr(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type for a timer object.
///
class Timer : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Timer(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

namespace unit {

/// Base type for a unit items.
class Item : public Node {
    AST_RTTI
public:
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// type: The type of the item, or null for no associated type.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// l: Location associated with the item.
    Item(shared_ptr<ID> id, const shared_ptr<Type> type = nullptr,
         const hook_list& hooks = hook_list(), const attribute_list& attrs = attribute_list(),
         const Location& l = Location::None);

    /// Returns the item's identifier. Even for anymous items, this will
    /// return an (internally generated) ID. Use anonymous() to check if no
    /// ID was passed to the constructor.
    shared_ptr<ID> id() const;

    /// Returns the item's type. If not overridden, this returns the type
    /// passed into the constructor, or set later via setType().
    virtual shared_ptr<spicy::Type> type();

    /// Overrides the type passed into the constructor. Note that this one
    /// will be returned by type() only if that method wasn't overridden.
    void setType(shared_ptr<spicy::Type> type);

    /// Return's the item's type as used inside the parse object. This is by
    /// default the type()'s fieldType() but may be adapted with, e.g., \a
    /// &convert.
    virtual shared_ptr<spicy::Type> fieldType();

    /// Returns the unit this item is part of, or null if none.
    shared_ptr<type::Unit> unit() const;

    /// Returns true if no ID was passed to the constructor.
    bool anonymous() const;

    /// Marks this field as an anonymos one (i.e., as if no ID has been
    /// passed to the ctor). Primarily for internal use.
    void setAnonymous();

    /// Returns true if there's another unit field with the same name.
    bool aliased() const;

    /// Marks this field as one sharing its name with another one in the same
    /// unit. Mainly for internal use by normalizer, who detects aliasing.
    void setAliased();

    /// Marks this item as created a by a ctor expression that didn't give it
    /// a name. Variables marked as such might treated differently when
    /// coercion union types.
    void setCtorNoName();

    /// Returns true if this item has been marked as created a by ctor
    /// expression.
    bool ctorNoName() const;

    /// Returns the item's scope. The scope may define identifier's local to
    /// expressions and hooks associated with the item. This method will
    /// return null until the ScopeBuilder has run.
    shared_ptr<Scope> scope() const;

    /// Returns the hooks associated with this item.
    hook_list hooks() const;

    /// Returns the attributes associated with the item.
    shared_ptr<AttributeSet> attributes() const;

    // Returns a Spicy expression associated with a property defined by the
    // item unit, or the module it's in. If the has a corresponding
    // attribute, that's checked first. The returned expression is the
    // most-specific one, i.e., the item is checked first, then the unit,
    // then the module. Returns null if none of them defines the
    // property/attribute.
    shared_ptr<spicy::Expression> inheritedProperty(const string& property);

    /// Disables code generation for this item's hooks. Calls to \a
    /// disableHook and \a enableHook must match for code generation to
    /// happen.
    void disableHooks();

    /// Enable code generation for this item's hooks. Calls to \a disableHook
    /// and \a enableHook must match for code generation to happen.
    void enableHooks();

    /// Returns true if the code generator should trigger the hooks for this
    /// item. By default, they are enabled.
    bool hooksEnabled();

    ACCEPT_VISITOR_ROOT();

    /// Sets the unit this item is part of. Only the unit itself should call
    /// this.
    void setUnit(type::Unit* unit);

protected:
    friend class type::Unit;

    /// Adds a hook to the item. Note that this must be called before we
    /// resolve the AST.
    void addHook(shared_ptr<spicy::Hook> hook);

private:
    bool _anonymous = false;
    bool _aliased = false;
    bool _ctor_no_name = false;
    int _do_hooks = 1;
    node_ptr<ID> _id;
    node_ptr<spicy::Type> _type;
    node_ptr<AttributeSet> _attrs;
    std::list<node_ptr<spicy::Hook>> _hooks;

    shared_ptr<Scope> _scope;

    // We don't use a shared ptr here because it's set from the Unit's ctor,
    // where we don't have one yet.
    type::Unit* _unit = 0;

    static int _id_counter;
};

namespace item {

/// Base class for unit fields parsed from the input.
class Field : public Item {
    AST_RTTI
public:
    /// The kind of a field defines whether it's to trigger during parsing or
    /// composing, or both.
    enum Kind {
        PARSE,         ///< A field to be parsed.
        COMPOSE,       ///< A field to be composed.
        PARSE_COMPOSE, ///< A field to be parsed and composed (default).
    };

    /// Returns the item's associated value, or null if none.
    shared_ptr<Expression> value() const;

    /// Returns the item's associated condition, or null if none.
    shared_ptr<Expression> condition() const;

    /// Returns the list of attached sinks.
    expression_list sinks() const;

    /// Returns the parameters passed to sub-type's parsing.
    expression_list parameters() const;

    /// Returns the kind of field.
    Kind kind() const;

    /// Sets the fields kind.
    void setKind(Kind kind);

    /// Returns true if the kind indicates that this fields is to be parsed.
    bool forParsing() const;

    /// Returns true if the kind indicates that this fields is to be composed.
    bool forComposing() const;

    /// Returns true if this is a transient field. A transient field is used
    /// for parsing but doesn't actually store the parsed value in the parse
    /// object.
    bool transient() const;

    shared_ptr<spicy::Type> fieldType() override;

    /// Create a field for parsing a type. This internally knows which field
    /// class to use for each type, and will create a corresponding instance.
    static shared_ptr<Field> createByType(shared_ptr<Type> type, shared_ptr<ID> id, Kind kind,
                                          shared_ptr<Expression> cond = nullptr,
                                          const hook_list& hooks = hook_list(),
                                          const attribute_list& attrs = attribute_list(),
                                          const expression_list& params = expression_list(),
                                          const expression_list& sinks = expression_list(),
                                          const Location& l = Location::None);


    /// Returns a parent field associated with this field, if any.
    Field* parent() const;

    /// Associates a parent field with this one.
    ///
    /// parent: The parent.
    void setParent(Field* parent);

    ACCEPT_VISITOR(Item);

protected:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// type: The type of the item.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// params: List of parameters passed to sub-type's parsing.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    Field(shared_ptr<ID> id, shared_ptr<spicy::Type> type, Kind kind,
          shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
          const attribute_list& attrs = attribute_list(),
          const expression_list& params = expression_list(),
          const expression_list& sinks = expression_list(), const Location& l = Location::None);

private:
    node_ptr<Expression> _cond;
    std::list<node_ptr<Expression>> _sinks;
    std::list<node_ptr<Expression>> _params;
    Field* _parent = nullptr;
    Kind _kind = PARSE_COMPOSE;
};

namespace field {

/// A unit field that's type we don't know yet: we need to look up an ID later.
class Unknown : public Field {
    AST_RTTI
public:
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// scope_id: The ID to lookup to resolve the field.
    ///
    /// kind: The kind of field.
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
    Unknown(shared_ptr<ID> id, shared_ptr<spicy::ID> scope_id, Kind kind,
            shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
            const attribute_list& attrs = attribute_list(),
            const expression_list& params = expression_list(),
            const expression_list& sinks = expression_list(), const Location& l = Location::None);

    /// Returns the ID to resolve the field.
    shared_ptr<spicy::ID> scopeID() const;

    ACCEPT_VISITOR(Field);

private:
    node_ptr<spicy::ID> _scope_id;
};

/// A unit field based on an atomic type.
class AtomicType : public Field {
    AST_RTTI
public:
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// kind: The kind of field.
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
    AtomicType(shared_ptr<ID> id, shared_ptr<spicy::Type> type, Kind kind,
               shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
               const attribute_list& attrs = attribute_list(),
               const expression_list& sinks = expression_list(),
               const Location& l = Location::None);

    ACCEPT_VISITOR(Field);
};

/// A unit field based on a sub-unit type.
class Unit : public Field {
    AST_RTTI
public:
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// kind: The kind of field.
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
    Unit(shared_ptr<ID> id, shared_ptr<spicy::Type> type, Kind kind,
         shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
         const attribute_list& attrs = attribute_list(),
         const expression_list& params = expression_list(),
         const expression_list& sinks = expression_list(), const Location& l = Location::None);

    ACCEPT_VISITOR(Field);
};

/// A constant unit field.
class Constant : public Field {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    ///
    /// const_: The constant's value.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// l: Location associated with the item.
    Constant(shared_ptr<ID> id, shared_ptr<spicy::Constant> const_, Kind kind,
             shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
             const attribute_list& attrs = attribute_list(),
             const expression_list& sinks = expression_list(), const Location& l = Location::None);

    /// Returns the constant.
    shared_ptr<spicy::Constant> constant() const;

    ACCEPT_VISITOR(Field);

private:
    node_ptr<spicy::Constant> _const;
};

/// A unit field defined by a ctor expression.
class Ctor : public Field {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    //
    /// ctor: The ctor expression for parsing the field.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    Ctor(shared_ptr<ID> id, shared_ptr<spicy::Ctor> ctor, Kind kind,
         shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
         const attribute_list& attrs = attribute_list(),
         const expression_list& sinks = expression_list(), const Location& l = Location::None);

    /// Returns the regular expression.
    shared_ptr<spicy::Ctor> ctor() const;

    ACCEPT_VISITOR(Field);

private:
    node_ptr<spicy::Ctor> _ctor;
};

/// Base class for container types that parse other fields recursively.
class Container : public Field {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    //
    /// field: The field to parse recursively.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    Container(shared_ptr<ID> id, shared_ptr<Field> field, Kind kind,
              shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
              const attribute_list& attrs = attribute_list(),
              const expression_list& sinks = expression_list(), const Location& l = Location::None);

    /// Returns the contained field.
    shared_ptr<Field> field() const;

    ACCEPT_VISITOR(Field);

private:
    node_ptr<Field> _field;
};

namespace container {

/// A list field
class List : public Container {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    //
    /// field: The field to parse recursively.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    List(shared_ptr<ID> id, shared_ptr<Field> field, Kind kind,
         shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
         const attribute_list& attrs = attribute_list(),
         const expression_list& sinks = expression_list(), const Location& l = Location::None);

    shared_ptr<spicy::Type> type() override;

    ACCEPT_VISITOR(Container);
};

// A vector field.
class Vector : public Container {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of the item. Can be null for anonymous items.
    //
    /// field: The field to parse as vector elements.
    ///
    /// length: An expression indicating how often field is to be repeated.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// attrs: Attributes associated with the item.
    ///
    /// sinks: Expressions referencing attached sinks, if any.
    ///
    /// l: Location associated with the item.
    Vector(shared_ptr<ID> id, shared_ptr<Field> field, shared_ptr<Expression> length, Kind kind,
           shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
           const attribute_list& attrs = attribute_list(),
           const expression_list& sinks = expression_list(), const Location& l = Location::None);

    /// Returns the expression indicating the length of the parsed vector.
    shared_ptr<Expression> length() const;

    shared_ptr<spicy::Type> type() override;

    ACCEPT_VISITOR(Container);

private:
    node_ptr<Expression> _length;
};
}

namespace switch_ {

class Case : public Node {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// exprs: Expression associated with the case.
    ///
    /// item: The item implementing the case.
    ///
    /// l: Location associated with the case.
    Case(const expression_list& exprs, shared_ptr<type::unit::item::Field> item,
         const Location& l = Location::None);

    /// Constructor.
    ///
    /// exprs: Expression associated with the case.
    ///
    /// item: A set of items implementing the case.
    ///
    /// l: Location associated with the case.
    Case(const expression_list& exprs, const unit_field_list& items,
         const Location& l = Location::None);

    /// Constructor for a default case.
    ///
    /// item: The item implementing the case.
    ///
    /// l: Location associated with the case.
    Case(shared_ptr<type::unit::item::Field> item, const Location& l = Location::None);

    /// Constructor for a default case.
    ///
    /// item: The item implementing the case.
    ///
    /// l: Location associated with the case.
    Case(const unit_field_list& items, const Location& l = Location::None);

    /// Returns the case's expression.
    expression_list expressions() const;

    /// Returns true if this is the default case.
    bool default_() const;

    /// Returns the case's implementation items.
    unit_field_list fields() const;

    /// Returns a name uniquely identifying this switch.
    std::string uniqueName();

    ACCEPT_VISITOR_ROOT();

private:
    bool _default = false;
    std::list<node_ptr<Expression>> _exprs;
    std::list<node_ptr<type::unit::item::Field>> _items;
};
}

/// Type for a unit switch item
class Switch : public Field {
    AST_RTTI
public:
    typedef std::list<shared_ptr<switch_::Case>> case_list;

    /// Constructor.
    ///
    /// expr: The switch's expression. Can be null for a look-ahead switch.
    ///
    /// cases: The list of switch cases.
    ///
    /// kind: The kind of field.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// l: Location associated with the item.
    Switch(shared_ptr<Expression> expr, const case_list& cases, Kind kind,
           shared_ptr<Expression> cond = nullptr, const hook_list& hooks = hook_list(),
           const Location& l = Location::None);

    /// Returns the switch's expression.
    shared_ptr<Expression> expression() const;

    /// Returns the list of cases.
    case_list cases() const;

    /// Returns true if there's no field storing information.
    bool noFields() const;

    /// Returns the case that a field is part of, or null if none.
    ///
    /// f: The field.
    shared_ptr<switch_::Case> case_(shared_ptr<type::unit::Item> f);

    /// Returns a name uniquely identifying this switch.
    std::string uniqueName();

    ACCEPT_VISITOR(Item);

private:
    node_ptr<Expression> _expr;
    std::list<node_ptr<switch_::Case>> _cases;
};
}

/// A user-defined unit variable.
class Variable : public Item {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared property. If it does not have a leading dot,
    /// that will be added.
    ///
    /// type: The variable's type.
    ///
    /// default_: An optional default value. This will be turned into a &default attribute.
    ///
    /// hooks: Hooks associated with this item.
    ///
    /// l: An associated location.
    Variable(shared_ptr<spicy::ID> id, shared_ptr<spicy::Type> type,
             shared_ptr<Expression> default_, const hook_list& hooks = hook_list(),
             const attribute_list& attrs = attribute_list(), const Location& l = Location::None);

    /// Returns the variable's default, or null if none. This is just a
    /// convinience method that returns the value of the items \c &default
    /// attribute.
    shared_ptr<Expression> default_() const;

    ACCEPT_VISITOR(Item);
};

/// A unit property.
class Property : public Item {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// prop: The property being declared.
    ///
    /// l: An associated location.
    Property(shared_ptr<Attribute> prop, const Location& l = Location::None);

    /// Returns the property.
    shared_ptr<Attribute> property() const;

    ACCEPT_VISITOR(Item);

private:
    node_ptr<Attribute> _property;
};

/// A unit-wide hook.
class GlobalHook : public Item {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of the hook.
    ///
    /// hooks: The hooks.
    ///
    /// l: An associated location.
    GlobalHook(shared_ptr<ID> id, hook_list& hooks, const Location& l = Location::None);

    ACCEPT_VISITOR(Item);
};
}
}

/// Type for units.
class Unit : public PacType, public trait::Parseable {
    AST_RTTI
public:
    typedef std::list<shared_ptr<unit::item::Field>> field_list;

    /// Constructor.
    ///
    /// items: The unit's items.
    ///
    /// l: Associated location.
    Unit(const parameter_list& params, const unit_item_list& items,
         const Location& l = Location::None);

    /// Constructor for a wildcard unit type matching any other.
    ///
    /// l: Associated location.
    Unit(const Location& l = Location::None);

    /// Returns the type's parameters.
    parameter_list parameters() const;

    /// Returns a list with type's parameter types.
    type_list parameterTypes() const;

    /// Returns the list of unit items.
    unit_item_list items() const;

    /// Returns a "flattened" list of all of the units items. This descends
    /// into items that recursively contain further items and adds them all
    /// to the list as well.
    unit_item_list flattenedItems() const;

    /// Returns a list of all fields. This is a convinience method that
    /// prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::Field>> fields() const;

    /// Returns a "flattened" list of all of the unit's fields. This descends
    /// into fields that recursively contain further items and adds them all
    /// to the list as well.
    std::list<shared_ptr<unit::item::Field>> flattenedFields() const;

    /// Returns a list of all variables. This is a convinience method that
    /// prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::Variable>> variables() const;

    /// Returns a list of all global-wide hooks. This is a convinience method
    /// that prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::GlobalHook>> globalHooks() const;

    /// Returns a list of all properties. This is a convinience method that
    /// prefilters all items for this tyoe,
    std::list<shared_ptr<unit::item::Property>> properties() const;

    /// Returns a list of all properties with a given name.
    std::list<shared_ptr<unit::item::Property>> properties(const string& name) const;

    /// Returns the property of a given name, or null if none. If there's
    /// more than one property with the given name, one of them is returned.
    shared_ptr<unit::item::Property> property(const string& prop) const;

    /// Returns the item of a given name, or null if there's no such item.
    ///
    /// id: The item to look up.
    shared_ptr<unit::Item> item(shared_ptr<ID> id) const;

    /// Returns the item of a given name, or null if there's no such item.
    ///
    /// name: The item to look up.
    shared_ptr<unit::Item> item(const std::string& name) const;

    /// Traverses an ID path to a sub-item and returns it. For example, \c
    /// a.b.c descends down field \c a, then \c b, and finally returns \c c
    /// there. Returns null for the item if the path is not valid. The second
    /// element of the returned tuple is a remaining string if we arrive at
    /// at leaf item before the path has been fully used (e.g., if with \c
    /// a.b.c, \c b is an atomic field, it would be set to \c c.), while the
    /// returned item is \c b.
    std::pair<shared_ptr<unit::Item>, string> path(const string& path);

    /// Returns the unit's scope. The scope may define identifier's local to
    /// expressions and hooks associated with the unit or any of its items.
    /// This method will return null until the ScopeBuilder has run.
    shared_ptr<Scope> scope() const;

    /// Returns the grammar for this type, if set. Intially, this is unset
    /// but will be initialized by the grammar builder.
    shared_ptr<Grammar> grammar() const;

    /// Returns true if parsing this unit type requires to buffer to the
    /// input until it's fully processed. This is initially always false, but
    /// the op-resolver pass will set it as neccessary based on the operations
    /// that are performed with the type.
    bool buffering() const;

    /// Enables buffering for this type as returned by buffering(). Mainly
    /// for internal use.
    void enableBuffering();

    /// Returns true if parsing this unit type requires to track a look-ahead
    /// symbol. This is initially always false, but it may be enabled if the
    /// look-ahead is determined to be needed.
    bool trackLookAhead() const;

    /// Returns true if this unit type has been exported. This is initially
    /// always false, but the normalizer pass will set it as neccessary based
    /// on the operations that are performed with the type.
    bool exported() const;

    /// Marks the type as being exported. Mainly for internal use.
    void setExported();

    /// Returns true if this unit type has been marked anonymous. This is
    /// typically the case for types construced implictly on the fly, like
    /// with a unit ctor.
    bool anonymous() const;

    /// Marks the unit type as anonymous.
    void setAnonymous();

    /// Returns true if all items have ctorNoName() set. As that must be the
    /// case for either all items or none, returning false means the latter.
    bool ctorNoNames() const;

    // Returns a Spicy expression associated with a property defined by the
    // unit, or the module it's in. If a item is specified and has a
    // corresponding attribute, that's checked as well. The returned
    // expression is the most-specific one, i.e., the item is checked first,
    // then the unit, then the module. Returns null if none of them defines
    // the property/attribute.
    shared_ptr<spicy::Expression> inheritedProperty(const string& property,
                                                     shared_ptr<type::unit::Item> item = nullptr);

    /// XXX Compare to production::supportsSynchronize.
    bool supportsSynchronize();

    bool _equal(shared_ptr<spicy::Type> other) const override;

    ACCEPT_VISITOR(Type);

protected:
    friend class passes::GrammarBuilder;

    void setGrammar(shared_ptr<Grammar> grammar);

    // Internal helper for travesePath().
    std::pair<shared_ptr<unit::Item>, string> path(string_list path, shared_ptr<Unit> current);

private:
    bool _buffering = false;
    bool _exported = false;
    bool _anonymous = false;

    std::list<node_ptr<type::function::Parameter>> _params;
    std::list<node_ptr<unit::Item>> _items;

    shared_ptr<Scope> _scope = nullptr;
    shared_ptr<Grammar> _grammar = nullptr;
};

/// A sink for parsing data with another parser.
class Sink : public PacType {
    AST_RTTI
public:
    /// Constructor.
    Sink(const Location& l = Location::None);
    ~Sink();

    ACCEPT_VISITOR(PacType);
};

////

/// Type representing an object embedded into a data stream.
class EmbeddedObject : public TypedPacType, public trait::Parseable {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// etype: The type of the embedded object.
    ///
    /// l: Associated location.
    EmbeddedObject(shared_ptr<Type> etype, const Location& l = Location::None);

    /// Constructor for a wildcard type.
    EmbeddedObject(const Location& l = Location::None);

    ACCEPT_VISITOR(TypedPacType);
};

/// Type representing a mark inside a bytes object.
class Mark : public PacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: Associated location.
    Mark(const Location& l = Location::None);

    ACCEPT_VISITOR(PacType);
};

/// Type representing an optional instance of another type.
class Optional : public TypedPacType {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// type: The type of the wrappend instance.
    ///
    /// l: Associated location.
    Optional(shared_ptr<Type> type, const Location& l = Location::None);

    /// Constructor for a wildcard type.
    Optional(const Location& l = Location::None);

    ACCEPT_VISITOR(TypedPacType);
};
}
}


#endif

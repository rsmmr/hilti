
#ifndef AST_TYPE_H
#define AST_TYPE_H

#include <util/util.h>

#include "node.h"
#include "mixin.h"

namespace ast {

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

/// A helper class for mixin::Function to describe a function paramemeter.
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

    /// Returns the name of the parameter. Null for return values.
    shared_ptr<ID> id() const { return _id; }

    /// Returns true if the parameter is marked constant.
    bool constant() const { return _constant; }

    /// Returns the type of the parameter.
    shared_ptr<Type> type() const { return _type; }

    /// Returns the default value of the parameter, or null if none.
    shared_ptr<Expression> default_() const { return _default_value; }

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

template<typename AstInfo>
class Result : public Parameter<AstInfo>
{
public:
    typedef typename AstInfo::type Type;

    /// Constructor for a return value.
    ///
    /// type: The type of the return value.
    ///
    /// constant: A flag indicating whether the return value is constant
    /// (i.e., a caller may not change its value.)
    ///
    /// l: A location associated with the expression.
    Result(shared_ptr<Type> type, bool constant, Location l=Location::None)
       : Parameter<AstInfo>(nullptr, type, constant, nullptr, l) {}
};

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
    typedef typename AstInfo::result Result;

    typedef std::list<shared_ptr<Parameter>> parameter_list;

    /// Constructor.
    ///
    /// target: The type we're mixed in with.
    ///
    /// result: The function return type.
    ///
    /// params: The function's parameters.
    Function(Type* target, shared_ptr<Result> result, const parameter_list& params);

    /// Returns the function's parameters.
    parameter_list parameters() const {
        parameter_list params;
        for ( auto p : _params )
            params.push_back(p);

        return params;
    }

    /// Returns the function's result type.
    shared_ptr<Result> result() const { return _result; }

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
    std::list<node_ptr<Parameter>> _params;
    node_ptr<Parameter> _result;
};

}

//////////////////////// Traits.

// Base class for types trait. We use trait to mark types that have certain
// properties.
//
// Note that Traits aren't (and can't) be derived from Node and thus aren't
// AST nodes.
class Trait
{
public:
    Trait() {}
    virtual ~Trait();
};

namespace trait {

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
template<class AstInfo>
class Type : public Base {
public:
    typedef typename AstInfo::type AIType;

    /// Constructor.
    ///
    /// type: The type the parameter specifies.
    Type(shared_ptr<AIType> type) { _type = type; }

    /// Returns the type the parameter specifies.
    shared_ptr<AIType> type() const { return _type; }

    bool _equal(shared_ptr<Base> other) const override {
       return _type->equal(std::dynamic_pointer_cast<Type>(other)->_type);
    }

private:
    node_ptr<AIType> _type;
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
template<typename AstInfo>
class Enum : public Base {
public:
    typedef typename AstInfo::id ID;

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
    node_ptr<ID> _label;
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
template<typename AstInfo>
class Parameterized : public virtual Trait
{
public:
    typedef typename AstInfo::type AIType;

    typedef std::list<shared_ptr<parameter::Base>> type_parameter_list;

    /// Compares the type with another for equivalence, which must be an
    /// instance of the same type class.
    bool equal(shared_ptr<AIType> other) const {
       auto pother = dynamic_cast<type::trait::Parameterized<AstInfo> *>(other.get());
       assert(pother);

       auto params = parameters();
       auto oparams = pother->parameters();

       if ( params.size() != oparams.size() )
           return false;

       auto i1 = params.begin();
       auto i2 = oparams.begin();

       for ( ; i1 != params.end(); ++i1, ++i2 ) {
           if ( ! (*i1 && *i2) )
               return false;

           if ( typeid(**i1) != typeid(**i2) )
               return false;

           if ( ! (*i1)->_equal(*i2) )
               return false;
       }

    return true;
    }

    /// Returns the type's parameters. Note we return this by value as the
    /// derived classes need to build the list on the fly each time so that
    /// potential updates to the members (in particular type resolving) get
    /// reflected.
    virtual type_parameter_list parameters() const = 0;
};

/// Trait class marking a composite type that has a series of subtypes.
template<typename AstInfo>
class TypeList : public virtual Trait
{
public:
    typedef typename AstInfo::type AIType;
    typedef std::list<shared_ptr<AIType>> type_list;

    /// Returns the ordered list of subtypes.
    virtual const type_list typeList() const = 0;
};

/// Trait class marking a type that provides iterators.
template<typename AstInfo>
class Iterable : public virtual Trait
{
public:
    typedef typename AstInfo::type AIType;

    /// Returns the type for an iterator over this type.
    virtual shared_ptr<AIType> iterType() = 0;

    /// Returns the type of elements when iterating over this type.
    virtual shared_ptr<AIType> elementType() = 0;
};

/// Trait class marking a type that can be hashed for storing in containers.
class Hashable : public virtual Trait
{
public:
};

/// Trait class marking a container type.
template<typename AstInfo>
class Container : public Iterable<AstInfo>
{
};

/// Returns true if type \a t has trait \a T.
template<typename T>
inline bool hasTrait(shared_ptr<NodeBase> t) { return std::dynamic_pointer_cast<T>(t) != 0; }

/// Dynamic-casts type \a t into trait class \a T.
///
/// \deprecated: Use checkedTrait or tryTrait instead.
template<typename T>
inline T* asTrait(NodeBase* t) { return dynamic_cast<T*>(t); }

/// Dynamic-casts type \a t into trait class \a T.
///
/// \deprecated: Use checkedTrait or tryTrait instead.
template<typename T>
inline T* asTrait(shared_ptr<NodeBase> t) { return std::dynamic_pointer_cast<T>(t); }

}

/// Dynamic-casts type \a t into trait class \a T. This version return null
/// if the cast fails.
template<typename T>
inline T* tryTrait(NodeBase* t) { return dynamic_cast<T*>(t); }

/// Dynamic-casts type \a t into trait class \a T. This version return null
/// if the cast fails.
template<typename T>
inline shared_ptr<T> tryTrait(shared_ptr<NodeBase> t) { return std::dynamic_pointer_cast<T>(t); }

/// Dynamic-casts type \a t into trait class \a T. This version aborts if the
/// cast fails.
///
/// Returns: The cast pointer.
template<typename T>
inline shared_ptr<T> checkedTrait(NodeBase* t) {
    auto c = std::dynamic_pointer_cast<T>(t);

    if ( ! c ) {
        fprintf(stderr, "internal error: ast::checkedTrait() failed; want '%s' but got a '%s'",
                typeid(T).name(), typeid(*t).name());
        abort();
    }

    return c;
}

/// Dynamic-casts type \a t into trait class \a T. This version aborts if the
/// cast fails.
///
/// Returns: The cast pointer.
template<typename T>
inline T* checkedTrait(shared_ptr<NodeBase> t) {
    auto c = dynamic_cast<T*>(t.get());

    if ( ! c ) {
        fprintf(stderr, "internal error: ast::checkedTrait() failed; want '%s' but got a '%s'",
                typeid(T).name(), typeid(*t.get()).name());
        abort();
    }

    return c;
}

//////////////////////// Implementations.

template<typename AstInfo>
inline mixin::Function<AstInfo>::Function(Type* target, shared_ptr<Result> result, const parameter_list& params)
    : __TYPE_MIXIN(target, this)
{
    for ( auto p : params )
        _params.push_back(p);

    for ( auto p : _params )
        target->addChild(p);

    _result = result;
    target->addChild(_result);
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

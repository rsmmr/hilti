
#ifndef BINPAC_CONSTANT_H
#define BINPAC_CONSTANT_H

#include <netinet/in.h>
#include <stdint.h>

#include <ast/constant.h>
#include <ast/visitor.h>

#include "common.h"

namespace binpac {

class Expression;

namespace type {
class Bitset;
class Enum;
}

/// Base class for constant nodes.
class Constant : public ast::Constant<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: An associated location.
    Constant(const Location& l = Location::None);

    /// Returns a readable representation of the constant.
    string render() override;

    ACCEPT_VISITOR_ROOT();
};

namespace constant {

/// Exception thrown when a constant cannot be parsed into the internal
/// representation.
class ConstantParseError : public ast::RuntimeError {
public:
    ConstantParseError(Constant* c, const string& msg);
    ConstantParseError(shared_ptr<Constant> c, const string& msg);
};

#if 0
/// AST node for a constant represented by an expression. The expression must
/// have a constant value, not suprsingly.
class Expression : public Constant
{
    AST_RTTI
public:
    /// Constructor.
    ///
    /// expr: The expression representing the constant's value. It's
    /// type must coerce to \a type if that's given.
    ///
    /// type: The type of the constant. If null, infered from \a expr.
    ///
    /// l: Associated location.
    Expression(shared_ptr<Type> type, shared_ptr<binpac::Expression> expr, const Location& l=Location::None);

    /// Returns the expression representing the constant's value.
    shared_ptr<binpac::Expression> expression() const;

    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    node_ptr<binpac::Expression> _expr;
    node_ptr<Type> _type;
};
#endif

/// AST node for a constant of type String.
class String : public ast::SpecificConstant<AstInfo, Constant, string> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// value: The value of the constant.
    ///
    /// l: An associated location.
    String(const string& value, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Integer.
class Integer : public ast::SpecificConstant<AstInfo, Constant, int64_t> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// value: The value of the constant.
    ///
    /// width: The bit width of the integer type.
    ///
    /// signed_: True if it's a signed integer.
    ///
    /// l: An associated location.
    Integer(int64_t value, int width, bool sign, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Bool.
class Bool : public ast::SpecificConstant<AstInfo, Constant, bool> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// value: The value of the constant.
    ///
    /// l: An associated location.
    Bool(bool value, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Tuple.
class Tuple : public Constant {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// elem: The constant's elements.
    ///
    /// l: An associated location.
    Tuple(const expression_list& elems, const Location& l = Location::None);

    /// Returns a list of the constant's elements.
    expression_list value() const;

    /// Returns the type of the constant.
    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    std::list<node_ptr<Expression>> _elems;
};

/// AST node for a constant of type Address.
class Address : public ast::SpecificConstant<AstInfo, Constant, string> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// addr: The address in dotted-quad (v4) or hex (v6) form.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the value is not valid.
    Address(const string& addr, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};


/// AST node for a constant of type Network.
class Network : public ast::SpecificConstant<AstInfo, Constant, string> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// net: The prefix in dotted-quad (v4) or hex (v6) form.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the value is not valid.
    Network(const string& net, const Location& l = Location::None);

    /// Constructor.
    ///
    /// net: The prefix in dotted-quad (v4) or hex (v6) form.
    ///
    /// width: The bitwidth of the subnet mask.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the value is not valid.
    Network(const string& net, int width, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Port.
class Port : public ast::SpecificConstant<AstInfo, Constant, string> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// port: A string of the form "<port>/<proto>".
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the value is not valid.
    Port(const string& port, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Bitset.
class Bitset : public Constant {
    AST_RTTI
public:
    typedef std::list<shared_ptr<ID>> bit_list;

    /// Constructor.
    ///
    /// bits: The name of the bits set. We reference each bit by the label
    /// defined in the bitset's tyoe.
    ///
    /// bstype: The type of the constant, which must be a type::BitSet.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if one of the \a bits is not defined by the type.
    Bitset(const bit_list& bits, shared_ptr<Type> bstype, const Location& l = Location::None);

    /// Returns the bits set in the constant.
    const bit_list& value() const;

    /// Returns the bit set's type.
    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    bit_list _bits;
    shared_ptr<type::Bitset> _bstype;
};

/// AST node for a constant of type Enum.
class Enum : public Constant {
    AST_RTTI
public:
    typedef std::list<string> bit_list;

    /// Constructor.
    ///
    /// value: The enum label.
    ///
    /// etype: The type of the constant, which must be a type::Enum.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the label is not defined by the type.
    Enum(shared_ptr<ID> label, shared_ptr<Type> etype, const Location& l = Location::None);

    /// Returns the enum label.
    shared_ptr<ID> value() const;

    /// Returns the enum's type.
    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    shared_ptr<ID> _value;
    shared_ptr<type::Enum> _etype;
};

/// AST node for a constant of type Double.
class Double : public ast::SpecificConstant<AstInfo, Constant, double> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// value: The value of the constant.
    ///
    /// l: An associated location.
    Double(double value, const Location& l = Location::None);

    ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Time.
class Time : public Constant {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// time: Seconds since epoch.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the \a time isn't parseable.
    Time(double time, const Location& l = Location::None);

    /// Constructor.
    ///
    /// time: Seconds since epoch.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the \a time isn't parseable.
    Time(uint64_t time, const Location& l = Location::None);

    /// Returns the time as nano seconds since the epoch.
    uint64_t value() const;

    /// Returns the type of the constant.
    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    uint64_t _nsecs;
};

/// AST node for a constant of type Interval.
class Interval : public Constant {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// interv: Seconds.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the \a interv isn't parseable.
    Interval(uint64_t interv, const Location& l = Location::None);

    /// Constructor.
    ///
    /// interv: Seconds.
    ///
    /// l: An associated location.
    ///
    /// Throws: ConstantParseError if the \a interv isn't parseable.
    Interval(double interv, const Location& l = Location::None);

    /// Returns the interval in nano seconds.
    const uint64_t value() const;

    /// Returns the type of the constant.
    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    uint64_t _nsecs;
};

/// AST node for a constant of type Optional.
class Optional : public Constant {
    AST_RTTI
public:
    /// Constructor for a set optional constant.
    ///
    /// expr: The expresssion to set the optional value to.
    ///
    /// l: An associated location.
    Optional(shared_ptr<Expression> expr, const Location& l = Location::None);

    /// Constructor for an unset of optional constant. This creates a
    /// wildcard optional that coerces to any other optional.
    ///
    /// l: An associated location.
    Optional(const Location& l = Location::None);

    /// Returns the optional's expression, or null if none for a wildcard
    /// constant.
    shared_ptr<Expression> value() const;

    /// Returns the type of the constant.
    shared_ptr<Type> type() const override;

    ACCEPT_VISITOR(Constant);

private:
    node_ptr<Expression> _expr;
};
}
}

#endif

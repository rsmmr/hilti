
#ifndef HILTI_CONSTANT_H
#define HILTI_CONSTANT_H

#include <netinet/in.h>
#include <stdint.h>

#include <ast/constant.h>
#include <ast/exception.h>

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

   /// Returns a fully flattened list of all atomic sub-expressions.
   ///
   /// Can be overridden by derived classes. The default returns just an
   /// empty list.
   virtual std::list<shared_ptr<hilti::Expression>> flatten();

   ACCEPT_VISITOR_ROOT();
};

namespace constant {

/// Exception thrown when a constant cannot be parsed into the internal
/// representation.
class ConstantParseError : public ast::RuntimeError
{
public:
   ConstantParseError(Constant* c, const string& msg)
       : ast::RuntimeError(msg, c) {}

   ConstantParseError(shared_ptr<Constant> c, const string& msg)
       : ast::RuntimeError(msg, c.get()) {}
};

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

/// AST node for a constant of type Label.
class Label : public ast::SpecificConstant<AstInfo, Constant, string>
{
public:
   /// Constructor.
   ///
   /// id: The name of the label.
   ///
   /// l: An associated location.
   Label(string id, const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, string>(id, shared_ptr<Type>(new type::Label()), l) {}

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

   /// Constructor for an unset of a particular type.
   ///
   /// t: The type of the unset value.
   ///
   /// l: An associated location.
   Unset(shared_ptr<Type> t, const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, bool>(false, t, l) {}

   ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type \c ref<*>. This can be only \c null.
class Reference : public Constant
{
public:
   /// Constructor. This creates a \c null constant of the given type.
   ///
   /// l: An associated location.
   Reference(shared_ptr<Type> type, const Location& l=Location::None);

   /// Constructor. This creates a \c null constant of wildcard type.
   ///
   /// l: An associated location.
   Reference(const Location& l=Location::None);

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override;

   ACCEPT_VISITOR(Constant);

private:
   node_ptr<Type> _type;
};

/// AST node for a constant of type \c caddr. This can be only \c null (which
/// technically is a references but coerces into this).
class CAddr : public Constant
{
public:
   /// Constructor. This creates a \c null constant.
   ///
   /// l: An associated location.
   CAddr(const Location& l=Location::None);

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override;

   ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Tuple.
class Tuple : public Constant
{
public:
   typedef std::list<shared_ptr<Expression>> element_list;

   /// Constructor.
   ///
   /// elem: The constant's elements.
   ///
   /// l: An associated location.
   Tuple(const element_list& elems, const Location& l=Location::None);

   /// Returns a list of the constant's elements.
   element_list value() const;

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override;

   std::list<shared_ptr<hilti::Expression>> flatten() override;

   ACCEPT_VISITOR(Constant);

private:
   std::list<node_ptr<Expression>> _elems;
};

/// Internal representation for IP addresses.
struct AddressVal {
    AddressVal() : family(Unset) {}
    // Used internally only.
    AddressVal(Constant* c, const string& addr);

    enum { Unset, IPv4, IPv6 } family; //! Indicates whether we have an IPv4 and IPv6 address.
    union {
        struct in_addr in4;            //! A IPv4 address if \c family is \c IPv4.
        struct in6_addr in6;           //! A IPv6 address if \c family is \c IPv6.
    } in;
};

/// AST node for a constant of type Address.
class Address : public Constant
{
public:
   /// Constructor.
   ///
   /// addr: The address in dotted-quad (v4) or hex (v6) form.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if \a addr isn't parseable.
   Address(const string& addr, const Location& l=Location::None);

   /// Returns the address.
   const AddressVal& value() const { return _addr; }

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override {
       return shared_ptr<Type>(new type::Address());
   }

   ACCEPT_VISITOR(Constant);

private:
   AddressVal _addr;
};

/// AST node for a constant of type Network.
class Network : public Constant
{
public:
   /// Constructor.
   ///
   /// addr: The prefix in dotted-quad (v4) or hex (v6) form.
   ///
   /// width: The width of the subnet.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a addr isn't parseable.
   Network(const string& prefix, int width, const Location& l=Location::None);

   /// Constructor.
   ///
   /// cidr: The network in CIDR notation.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a cidr isn't parseable.
   Network(const string& cidr, const Location& l=Location::None);

   /// Constructor.
   ///
   /// addr: The address representing the network prefix.
   ///
   /// l: An associated location.
   Network(const AddressVal& addr, int width, const Location& l=Location::None);

   /// Returns the network prefix.
   const AddressVal& prefix() const { return _addr; }

   /// Returns the network width.
   int width() const { return _width; }

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override {
       return shared_ptr<Type>(new type::Network());
   }

   ACCEPT_VISITOR(Constant);

private:
   AddressVal _addr;
   int _width;
};

/// Internal representation for ports.
struct PortVal {
    enum Proto { TCP, UDP, ICMP } proto; //! The port's protocol.
    uint16_t port;           //! The port number.
};

/// AST node for a constant of type Port.
class Port : public Constant
{
public:
   /// Constructor.
   ///
   /// port: A string of the form "<port>/<proto>".
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a port isn't parseable.
   Port(const string& addr, const Location& l=Location::None);

   /// Constructor.
   ///
   /// port: The port number.
   ///
   /// proto: The ports protocol.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a port isn't parseable.
   Port(int port, constant::PortVal::Proto proto, const Location& l=Location::None);

   /// Returns the port.
   const PortVal& value() const { return _port; }

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override {
       return shared_ptr<Type>(new type::Port());
   }

   ACCEPT_VISITOR(Constant);

private:
   PortVal _port;
};

/// AST node for a constant of type Bitset.
class Bitset : public Constant
{
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
   Bitset(const bit_list& bits, shared_ptr<Type> bstype, const Location& l=Location::None);

   /// Returns the bits set in the constant.
   const bit_list& value() const { return _bits; }

   /// Returns the bit set's type.
   shared_ptr<Type> type() const override { return _bstype; }

   ACCEPT_VISITOR(Constant);
private:
   bit_list _bits;
   shared_ptr<type::Bitset> _bstype;
};

/// AST node for a constant of type Enum.
class Enum : public Constant
{
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
   Enum(shared_ptr<ID> label, shared_ptr<Type> etype, const Location& l=Location::None);

   /// Returns the bits set in the constant.
   shared_ptr<ID> value() const { return _value; }

   /// Returns the enum's type.
   shared_ptr<Type> type() const override { return _etype; }

   ACCEPT_VISITOR(Constant);
private:
   shared_ptr<ID> _value;
   shared_ptr<type::Enum> _etype;
};


/// AST node for a constant of type Double.
class Double : public ast::SpecificConstant<AstInfo, Constant, double>
{
public:
   /// Constructor.
   ///
   /// value: The value of the constant.
   ///
   /// l: An associated location.
   Double(double value, const Location& l=Location::None)
       : ast::SpecificConstant<AstInfo, Constant, double>(value, shared_ptr<Type>(new type::Double()), l) {}

   ACCEPT_VISITOR(Constant);
};

/// AST node for a constant of type Time.
class Time : public Constant
{
public:
   /// Constructor.
   ///
   /// time: Seconds since epoch.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a time isn't parseable.
   Time(double time, const Location& l=Location::None) : Constant(l) {
       _time = (uint64_t) (time * 1e9);
   }

   /// Constructor.
   ///
   /// time: Seconds since epoch.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a time isn't parseable.
   Time(uint64_t time, const Location& l=Location::None) : Constant(l) {
       _time = (uint64_t) (time * 1e9);
   }

   /// Constructor.
   ///
   /// time: Nano seconds since epoch.
   ///
   /// takes_nsecs: Just a marker that this methods takes nsecs.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a time isn't parseable.
   Time(uint64_t time, bool takes_nsecs, const Location& l=Location::None) : Constant(l) {
       _time = time;
   }

   /// Returns the time as nano seconds since the epoch.
   uint64_t value() const { return _time; }

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override {
       return shared_ptr<Type>(new type::Time());
   }

   ACCEPT_VISITOR(Constant);

private:
   uint64_t _time;
};

/// AST node for a constant of type Interval.
class Interval : public Constant
{
public:
   /// Constructor.
   ///
   /// interv: Seconds.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a interv isn't parseable.
   Interval(uint64_t interv, const Location& l=Location::None) : Constant(l) {
       _interv = (uint64_t) (interv * 1e9);
   }

   /// Constructor.
   ///
   /// interv: seconds.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a interv isn't parseable.
   Interval(double interv, const Location& l=Location::None) : Constant(l) {
       _interv = (uint64_t) (interv * 1e9);
   }

   /// Constructor.
   ///
   /// interv: Nano seconds.
   ///
   /// nsecs: Just a marker that this methods takes nsecs.
   ///
   /// l: An associated location.
   ///
   /// Throws: ConstantParseError if the \a interv isn't parseable.
   Interval(uint64_t interv, bool takes_nsecs, const Location& l=Location::None) : Constant(l) {
       _interv = interv;
   }

   /// Returns the interval in nano seconds.
   uint64_t value() const { return _interv; }

   /// Returns the type of the constant.
   shared_ptr<Type> type() const override {
       return shared_ptr<Type>(new type::Interval());
   }

   ACCEPT_VISITOR(Constant);

private:
   uint64_t _interv;
};

/// AST node for a union constructor.
class Union : public Constant
{
public:
   /// Constructor.
   ///
   /// utype: The type of the union. Can be null to derive automatically;
   /// then the type will be a union with a single field determined by
   /// *field* and *value*.
   ///
   /// field: The name of the field to initialize. Can be null for an
   /// anoymous field, with the field value's type determing what to set.
   ///
   /// value: The value to initialize the field with. Can be null for a union
   /// with all fields left unset.
   ///
   /// l: An associated location.
   Union(shared_ptr<Type> utype, shared_ptr<ID> field, shared_ptr<Expression> value, const Location& l=Location::None);

   /// Returns the type of the constructed union. If not explicity set, this
   /// will be derived from the value. If no value has been set either, this
   /// will be a wildcard union type.
   shared_ptr<Type> type() const override;

   /// Returns true if the type is derived rather than provied.
   bool typeDerived() const { return _type.get(); }

   /// Returns the name of the initialized field if any, or null if
   /// anonymous.
   shared_ptr<ID> id() const { return _id; }

   /// Returns the initilization value if any, or null if none.
   shared_ptr<Expression> expression() const { return _expr; }

   std::list<shared_ptr<hilti::Expression>> flatten() override;

   ACCEPT_VISITOR(Constant);

private:
   node_ptr<Type> _type;
   node_ptr<ID> _id;
   node_ptr<Expression> _expr;
};


}

}

#endif

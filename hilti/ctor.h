
#ifndef HILTI_CTOR_H
#define HILTI_CTOR_H

#include <stdint.h>

#include <ast/ctor.h>

#include "common.h"
#include "scope.h"
#include "type.h"
#include "visitor-interface.h"

namespace hilti {

/// Base class for ctor nodes. A ctor instantiates a HeapType.
class Ctor : public ast::Ctor<AstInfo>
{
public:
   /// Constructor.
   ///
   /// l: An associated location.
   Ctor(const Location& l=Location::None)
       : ast::Ctor<AstInfo>(l) {}

   /// Returns a fully flattened list of all atomic sub-expressions.
   ///
   /// Can be overridden by derived classes. The default returns just an
   /// empty list.
    virtual std::list<shared_ptr<hilti::Expression>> flatten();

   ACCEPT_VISITOR_ROOT();
};

namespace ctor {

/// AST node for a bytes constructor.
class Bytes : public Ctor
{
public:
   /// Constructor.
   ///
   /// b: The value to initialize the bytes object with.
   ///
   /// l: An associated location.
   Bytes(const string& b, const Location& l=Location::None) : Ctor(l) {
       _value = b;
   }

   /// Returns the initialization value.
   const string& value() const { return _value; }

   /// Returns the type of the constructed object.
   shared_ptr<Type> type() const override;

   ACCEPT_VISITOR(Ctor);

private:
   string _value;
};

/// AST node for a list constructor.
class List : public Ctor
{
public:
   typedef std::list<node_ptr<Expression>> element_list;

   /// Constructor.
   ///
   /// etype: The type of the the list's elements.
   ///
   /// elems: The elements for the instance being constructed.
   ///
   /// l: An associated location.
   List(shared_ptr<Type> etype, const element_list& elems, const Location& l=Location::None);

   /// Returns the initialization value.
   const element_list& elements() const { return _elems; }

   /// Returns the type of the constructed object. If the container has
   /// elements, the type will infered from those. If not, it will be a
   /// wildcard type.
   shared_ptr<Type> type() const { return _type; }

   std::list<shared_ptr<hilti::Expression>> flatten() override;

   ACCEPT_VISITOR(Ctor);

private:
   node_ptr<Type> _type;
   element_list _elems;
};

/// AST node for a vector constructor.
class Vector : public Ctor
{
public:
   typedef std::list<node_ptr<Expression>> element_list;

   /// Constructor.
   ///
   /// etype: The type of the the vector's elements.
   ///
   /// elems: The elements for the instance being constructed.
   ///
   /// l: An associated location.
   Vector(shared_ptr<Type> etype, const element_list& elems, const Location& l=Location::None);

   /// Returns the initialization value.
   const element_list& elements() const { return _elems; }

   /// Returns the type of the constructed object. If the container has
   /// elements, the type will infered from those. If not, it will be a
   /// wildcard type.
   shared_ptr<Type> type() const { return _type; }

   std::list<shared_ptr<hilti::Expression>> flatten() override;

   ACCEPT_VISITOR(Ctor);

private:
   node_ptr<Type> _type;
   element_list _elems;
};

/// AST node for a set constructor.
class Set : public Ctor
{
public:
   typedef std::list<node_ptr<Expression>> element_list;

   /// Constructor.
   ///
   /// etype: The type of the the set's elements.
   ///
   /// elems: The elements for the instance being constructed.
   ///
   /// l: An associated location.
   Set(shared_ptr<Type> etype, const element_list& elems, const Location& l=Location::None);

   /// Constructor.
   ///
   /// dummy: Dummy flag that differentiates this ctor from the one giving
   /// the element type. The value is ignored.
   ///
   /// stype: The type of the set, which must be \a type::Set.
   ///
   /// elems: The elements for the instance being constructed.
   ///
   /// l: An associated location.
   Set(bool dummy, shared_ptr<Type> stype, const element_list& elems, const Location& l=Location::None);

   /// Returns the initialization value.
   const element_list& elements() const { return _elems; }

   /// Returns the type of the constructed object. If the container has
   /// elements, the type will infered from those. If not, it will be a
   /// wildcard type.
   shared_ptr<Type> type() const { return _type; }

   std::list<shared_ptr<hilti::Expression>> flatten() override;

   ACCEPT_VISITOR(Ctor);

private:
   node_ptr<Type> _type;
   element_list _elems;
};

/// AST node for a list constructor.
class Map : public Ctor
{
public:
   typedef std::pair<node_ptr<Expression>,node_ptr<Expression>> element;
   typedef std::list<element> element_list;

   /// Constructor.
   ///
   /// ktype: The type of the map's index values.
   ///
   /// vtype: The type of the map's values.
   ///
   /// elems: The elements for the instance being constructed.
   ///
   /// l: An associated location.
   Map(shared_ptr<Type> ktype, shared_ptr<Type> vtype, const element_list& elems, const Location& l=Location::None);

   /// Constructor.
   ///
   /// mtype: The type of the map, which must be \a type::Map.
   ///
   /// elems: The elements for the instance being constructed.
   ///
   /// l: An associated location.
   Map(shared_ptr<Type> mtype, const element_list& elems, const Location& l=Location::None);

   /// Returns the initialization value.
   const element_list& elements() const { return _elems; }

   /// Returns the type of the constructed object. If the container has
   /// elements, the type will infered from those. If not, it will be a
   /// wildcard type.
   shared_ptr<Type> type() const { return _type; }

   std::list<shared_ptr<hilti::Expression>> flatten() override;

   ACCEPT_VISITOR(Ctor);

private:
   node_ptr<Type> _type;
   element_list _elems;
};

/// AST node for a regexp constructor.
class RegExp : public Ctor
{
public:
   /// A pattern is a tuple of two strings. The first element is the regexp
   /// itself, and the second a string with optional patterns flags.
   /// Currently, no flags are supported though.
   typedef std::pair<string, string> pattern;

   /// A list of patterns.
   typedef std::list<pattern> pattern_list;

   /// Constructor.
   ///
   /// patterns: List of patterns.
   ///
   /// l: An associated location.
   RegExp(const pattern_list& patterns, const Location& l=Location::None);

   /// Returns the pattern.
   const pattern_list& patterns() const { return _patterns; }

   /// Returns the type of the constructed object. Pattern constants are
   /// always of type \c regexp<>. To add further type attributes, they need
   /// to be coerced to a regexp type that has them.
   shared_ptr<Type> type() const { return _type; }

   ACCEPT_VISITOR(Ctor);

private:
   node_ptr<Type> _type;
   pattern_list _patterns;
};


}

}

#endif

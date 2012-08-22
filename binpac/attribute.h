
#ifndef BINPAC_ATTRIBUTE_H
#define BINPAC_ATTRIBUTE_H

#include <map>

#include <ast/node.h>
#include <ast/visitor.h>

#include "common.h"

using namespace binpac;

namespace binpac {

/// An key/value attribute.
class Attribute : public Node
{
public:
    /// Constructor.
    ///
    /// key: The key value. If it starts with an ampersand, that will be
    /// removed before storing it.
    ///
    /// value: The value, if any.
    ///
    /// internal: Mark this attribute as internal. The meaning is left to the
    /// interpretation of the user, but for example, internal type attributes
    /// won't be printed by the printer.
    ///
    /// l: Associated location.
    Attribute(const string& key, shared_ptr<Expression> value = nullptr, bool internal=false, const Location& l=Location::None);

    /// Returns the attribute's key (without any leading ampersand).
    const string& key() const;

    /// Returns the attribute's value, or null if not set.
    shared_ptr<Expression> value() const;

    /// Sets the attributes value.
    void setValue(shared_ptr<Expression> expr);

    /// Returns whether this is an internal attribute.
    bool internal() const;

    ACCEPT_VISITOR_ROOT();

private:
    string _key;
    node_ptr<Expression> _value;
    bool _internal;
};

/// A collection of attributes.
class AttributeSet : public Node
{
public:
    /// Ctor.
    ///
    /// l: Associated location.
    AttributeSet(const Location& l=Location::None) : ast::Node<AstInfo>(l) {}

    /// Ctor.
    ///
    /// attrs: Attributes to initialize the set with.
    ///
    /// l: Associated location.
    AttributeSet(const attribute_list& attrs, const Location& l=Location::None);

    /// Inserts an attribute into the set. If the same key already exists,
    /// it's overwritten.
    ///
    /// Returns: The previous attribute with the same key, if any.
    shared_ptr<Attribute> add(shared_ptr<Attribute> attr);

    /// Removes an attribute from the set. Returns the old attribute, or null
    /// if none.
    shared_ptr<Attribute> remove(const string& key);

    /// Returns true if the set contains an attribute of the given key.
    bool has(const string& key) const;

    /// Returns an attribute of a given key, or null if that key doesn't
    /// exist.
    shared_ptr<Attribute> lookup(const string& key) const;

    /// Returns a set of all attributes.
    std::list<shared_ptr<Attribute>> attributes() const;

    /// Returns the number of attributes defined.
    size_t size() const;

    AttributeSet& operator=(const attribute_list& attrs);

    ACCEPT_VISITOR_ROOT();

private:
    std::map<string, node_ptr<Attribute>> _attrs;
};

}

#endif

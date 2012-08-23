//
// Meta information that can be associated with AST nodes.

#ifndef AST_META_INFO_H
#define AST_META_INFO_H

#include <map>
#include <list>

#include <util/util.h>

#include "common.h"

namespace ast {

// A meta-information node that comes with no further value.
class MetaNode
{
public:
    /// Constructor.
    ///
    /// name: The name of the meta node.
    MetaNode(const string& name);
    ~MetaNode();

    /// Returns the node's name.
    const string& name() const;

    virtual string render() const;

private:
    string _name;
};

// A templated meta node that stores a value of a given type.
template<typename T>
class MetaValueNode : public MetaNode
{
public:
    /// Constructor.
    ///
    /// name: The name of the node.
    ///
    /// value: The associated value.
    MetaValueNode(const string& name, const T value) : _value(value) {}

    /// Returns the node's value.
    const T& value() const { return _value; }

    string render() const override {
        return util::fmt("%%%s=%s", name(), string(_value));
    }

private:
    T _value;

};

// A collection of meta-information nodes. The collection can store multiple
// nodes with the same key.
class MetaInfo
{
public:
    MetaInfo();
    ~MetaInfo();

    /// Returns true if there's a least one node with a given name.
    bool has(const string& key);

    /// Looks up a meta-node of a specific name. Returns the node if found,
    /// or null if not. If there are more than one node with that name, it's
    /// undefined which one is returned; use lookupAll() to get all matching
    /// nodes.
    shared_ptr<MetaNode> lookup(const string& key) const;

    /// Looks up all meta-nodes of a specific name. Returns an empty list if
    /// none are found.
    std::list<shared_ptr<MetaNode>> lookupAll(const string& key) const;

    /// Adds a meta-node to the collection.
    ///
    /// n: The node.
    void add(shared_ptr<MetaNode> n);

    /// Removes all meta-node of a given name. A no-op if no such node
    /// exists.
    void remove(const string& key);

    /// Removes a specific meta-node. It's an internal error if that node is
    /// not part of the collection.
    void remove(shared_ptr<MetaNode> n);

    /// Returns the number of meta-noces.
    size_t size() const;

    operator string() const;

private:
    typedef std::multimap<string, shared_ptr<MetaNode>> node_map;
    node_map _nodes;
};

}

#endif


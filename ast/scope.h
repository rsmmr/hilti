
#ifndef AST_SCOPE_H
#define AST_SCOPE_H

#include <iostream>
#include <list>
#include <map>
#include <set>
#include <typeinfo>

#include "common.h"
#include "util/util.h"

namespace ast {

/// A scope. A scope is essentially one level of a symbol table. It may have
/// both parent and child scopes and support lookups of scoped and non-scoped
/// IDs. With each ID, the scope associates a value of customizable type.
/// Typically, this will be an Expression.
template <typename AstInfo>
class Scope : public std::enable_shared_from_this<Scope<AstInfo>> {
public:
    typedef typename AstInfo::id ID;
    typedef shared_ptr<typename AstInfo::scope_value> Value;

private:
    typedef std::map<string, shared_ptr<Scope<AstInfo>>> scope_map;
    typedef std::map<string, shared_ptr<std::list<Value>>> value_map;

    struct Data {
        shared_ptr<ID> id = nullptr;
        scope_map childs;
        value_map values;
    };

public:
    /// Constructor.
    ///
    /// parent: A parent scope. If an ID is not found in the current scope,
    /// lookup() will forward recursively to parent scopes.
    Scope(shared_ptr<Scope> parent = nullptr)
    {
        _parent = parent;
        _data = std::make_shared<Data>();
    }

    ~Scope()
    {
    }

    /// Associates an ID with the scope.
    void setID(shared_ptr<ID> id)
    {
        _data->id = id;
    }

    /// Returns an associated ID, or null if none.
    shared_ptr<ID> id() const
    {
        return _data->id;
    }

    /// Returns the scope's parent, or null if none.
    shared_ptr<Scope> parent() const
    {
        return _parent;
    }

    // Sets the scope's parent.
    //
    // parent: The parent.
    void setParent(shared_ptr<Scope> parent)
    {
        _parent = parent;
    }

    /// Adds a child scope.
    ///
    /// id: The name of child's scope. If that's, e.g., \c Foo, lookup() now
    /// searches there when \c Foo::bar is queried.
    ///
    /// child: The child scope.
    void addChild(shared_ptr<ID> id, shared_ptr<Scope> child)
    {
        _data->childs.insert(typename scope_map::value_type(id->pathAsString(), child));
        // TODO: Should this set chold->parent? We might need to use new the
        // alias support more if changed.
    }

    /// Returns the complete mapping of identifier to values. Note that
    /// because one can't insert scoped identifiers, all the strings represent
    /// identifier local to this scope.
    const value_map& map() const
    {
        return _data->values;
    }

    /// Inserts an identifier into the scope. If the ID already exists, the
    /// value is added to its list.
    ///
    /// id: The ID. It must not be scoped.
    ///
    /// value: The value to associate with the ID.
    ///
    /// use_scope: If true, the name will be inserted with it's
    /// fully-qualified ID right into this scope. Look-ups will only find
    /// that if they are likewise fully-qualified. If false, any scope
    /// qualifiers are ignored (i.e., we assume the ID is local to this
    /// scope).
    bool insert(shared_ptr<ID> id, Value Value, bool use_scope = false);

    /// Lookups an ID.
    ///
    /// id: The ID, which may be scoped or unscoped.
    ///
    /// traverse: If true, the lookup traverses parent and child scopes as appropiate.
    ///
    /// Returns: The list of values associated with the ID, or an empty list
    /// if it was not found.
    std::list<Value> lookup(shared_ptr<ID> id, bool traverse = true) const
    {
        auto val = find(id, traverse);
        if ( val.size() )
            return val;

        return _parent && traverse ? _parent->lookup(id, traverse) : val;
    }

    /// Lookups an ID under the assumption that is must only have at max 1
    /// value associated with it. If it has more, that's an internal error
    /// and aborts execution.
    ///
    /// id: The ID, which may be scoped or unscoped.
    ///
    /// traverse: If true, the lookup traverses parent and child scopes as appropiate.
    ///
    /// Returns: The value associated with the ID, or null if it was not
    /// found.
    const Value lookupUnique(shared_ptr<ID> id, bool traverse = true) const
    {
        auto vals = lookup(id, traverse);
        assert(vals.size() <= 1);
        return vals.size() ? vals.front() : nullptr;
    }

    /// Returns true if an ID exists.
    ///
    /// id: The ID, which may be scoped or unscoped.
    ///
    /// traverse: If true, the lookup traverses parent and child scopes as
    /// appropiate.
    ///
    bool has(shared_ptr<ID> id, bool traverse = true) const
    {
        auto vals = find(id, traverse);
        if ( vals.size() )
            return true;

        return _parent && traverse ? _parent->has(id, traverse) : false;
    }

    /// Removes a local ID from the scope.
    ///
    /// id: The ID to remove. It must not be scoped.
    bool remove(shared_ptr<ID> id);

    /// Removes all entries from the scope.
    void clear();

    /// Returns an alias to the current scope. An alias scope reflects any
    /// changed made later to the original scope, however it maintains its
    /// own parent scope pointer and can thus be linked into a different
    /// module.
    ///
    /// parent: The parent for the alias scope, or null if none.
    ///
    /// Returns: A new scope that's an alias to the current one.
    shared_ptr<typename AstInfo::scope> createAlias(
        shared_ptr<typename AstInfo::scope> parent = nullptr) const
    {
        auto alias = std::make_shared<typename AstInfo::scope>(parent);
        alias->_data = _data;
        return alias;
    }

    /// XXX
    void mergeFrom(shared_ptr<typename AstInfo::scope> other)
    {
        for ( auto i : other->_data->values ) {
            if ( _data->values.find(i.first) == _data->values.end() )
                _data->values.insert(i);
        }
    }

    /// Dumps out a debugging representation of the scope's binding.
    ///
    /// out: The stream to use.
    void dump(std::ostream& out);

private:
    // Internal helpers doing the lookip opeations.
    inline std::list<Value> find(shared_ptr<ID> id, bool traverse) const;
    inline std::list<Value> find(typename ID::component_list::const_iterator begin,
                                 typename ID::component_list::const_iterator end,
                                 bool traverse) const;

    // Recursive version of dump().
    void dump(std::ostream& out, int level, std::set<const Scope*>* seen);

    shared_ptr<Data> _data;
    shared_ptr<Scope<AstInfo>> _parent;
};

template <typename AstInfo>
inline bool Scope<AstInfo>::insert(shared_ptr<ID> id, Value value, bool use_scope)
{
    auto name = use_scope ? id->pathAsString() : id->name();
    auto i = _data->values.find(name);

    if ( i != _data->values.end() )
        i->second->push_back(value);

    else {
        auto list = std::make_shared<std::list<Value>>();
        list->push_back(value);
        _data->values.insert(typename value_map::value_type(name, list));
    }

    return true;
}

template <typename AstInfo>
inline bool Scope<AstInfo>::remove(shared_ptr<ID> id)
{
    auto i = _data->values.find(id->pathAsString());
    if ( i == _data->values.end() )
        return false;

    _data->values.erase(i);
    return true;
}

template <typename AstInfo>
inline void Scope<AstInfo>::clear()
{
    _data->childs.clear();
    _data->values.clear();
}

template <typename AstInfo>
inline std::list<shared_ptr<typename AstInfo::scope_value>> Scope<AstInfo>::find(
    typename ID::component_list::const_iterator begin,
    typename ID::component_list::const_iterator end, bool traverse) const
{
    if ( begin == end )
        return std::list<Value>();

    // Ignore the initial component if it's our own scope.
    if ( id() && ::util::strtolower(*begin) == ::util::strtolower(id()->name()) )
        ++begin;

    if ( begin == end )
        return std::list<Value>();

    // See if it directly references a value.
    auto second = begin;

    if ( ++second == end ) {
        auto val = _data->values.find(*begin);

        if ( val != _data->values.end() )
            return *val->second;
    }

    if ( ! traverse )
        // Not found.
        return std::list<Value>();

    // Try lookups in child scopes of successive "left-anchored" subpaths,
    // starting with most specific.
    for ( auto j = end; j != begin; --j ) {
        auto id = ::util::strjoin(begin, j, "::");
        auto child = _data->childs.find(id);

        if ( child != _data->childs.end() )
            // Found a child that matches our subpath.
            return child->second->find(j, end, traverse);
    }

    // Not found.
    return std::list<Value>();
}

template <typename AstInfo>
inline std::list<shared_ptr<typename AstInfo::scope_value>> Scope<AstInfo>::find(
    shared_ptr<ID> id, bool traverse) const
{
    // Try a direct lookup on the full path first.
    // TODO: I believe we can skip this now that the other find() tries the full path.
    auto i = _data->values.find(id->pathAsString());
    if ( i != _data->values.end() )
        return *i->second;

    // Now try the path-based lookup.
    const typename ID::component_list& path = id->path();
    return find(path.begin(), path.end(), traverse);
}

template <typename AstInfo>
inline void Scope<AstInfo>::dump(std::ostream& out)
{
    std::set<const Scope*> seen;
    return dump(out, 0, &seen);
}

template <typename AstInfo>
inline void Scope<AstInfo>::dump(std::ostream& out, int level, std::set<const Scope*>* seen)
{
    string indent = "";

    for ( int i = 0; i < level; ++i )
        indent += "    ";

    if ( seen->find(this) != seen->end() ) {
        out << indent << "  (suppressed, already dumped)" << std::endl;
        return;
    }

    seen->insert(this);

    out << indent << "* Scope" << this;

    if ( _parent )
        out << " / "
            << "has parent scope " << _parent;
    else
        out << " / "
            << "no parent scope" << _parent;

    out << std::endl;

    for ( auto v : _data->values ) {
        out << indent << "  " << string(v.first) << " -> ";

        bool first = true;

        for ( auto i : *v.second ) {
            if ( ! first )
                out << indent << "    ";

            out << string(*i) << " " << std::endl;
            first = false;
        }
    }

    for ( auto c : _data->childs ) {
        out << indent << "  " << c.first << std::endl;
        c.second->dump(out, level + 1, seen);
    }
}
}
#endif

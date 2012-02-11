
#ifndef AST_SCOPE_H
#define AST_SCOPE_H

#include <map>
#include <typeinfo>

#include "common.h"

namespace ast {

/// A scope. A scope is essentially one level of a symbol table. It may have
/// both parent and child scopes and support lookups of scoped and non-scoped
/// IDs. With each ID, the scope associates a value of customizable type.
/// Typically, this will be an Expression.
template<typename AstInfo>
class Scope
{
public:
   typedef typename AstInfo::id ID;
   typedef shared_ptr<typename AstInfo::scope_value> Value;

   /// Constructor.
   ///
   /// parent: A parent scope. If an ID is not found in the current scope,
   /// lookup() will forward recursively to parent scopes.
   Scope(shared_ptr<Scope> parent = nullptr) {
       _parent = parent;
   }

   ~Scope() {}

   /// Returns the scope's parent, or null if none.
   shared_ptr<Scope> parent() const { return _parent; }

   // Sets the scope's parent.
   //
   // parent: The parent.
   void setParent(shared_ptr<Scope> parent) { _parent = parent; }

   typedef std::map<string, shared_ptr<Scope<AstInfo>>> scope_map;

   /// Adds a child scope.
   ///
   /// id: The name of child's scope. If that's, e.g., \c Foo, lookup() now
   /// searches there when \c Foo::bar is queried.
   ///
   /// child: The child scope.
   void addChild(shared_ptr<ID> id, shared_ptr<Scope> child) {
       assert(! id->isScoped());
       _childs.insert(typename scope_map::value_type(id->name(), child));
   }

   typedef std::map<string, Value> value_map;

   /// Returns the complete mapping of identifier to values. Note that
   /// because one can't insert scoped identifiers, all the strings represent
   /// identifier local to this scope.
   const value_map& map() const { return _values; }

   /// Inserts an identifier into the scope.
   ///
   /// id: The ID. It must not be scoped.
   /// value: The value to associate with the ID.
   bool insert(shared_ptr<ID> id, Value Value); // must not be scoped.

   /// Look ups an ID.
   ///
   /// id: The ID, which may be scoped or unscoped.
   ///
   /// traverse: If true, the lookup traverses parent and child scopes as appropiate.
   ///
   /// Returns: The value associated with the ID, or null if it was not
   /// found.
   const Value lookup(shared_ptr<ID> id, bool traverse=true) const {
       auto val = find(id, traverse);
       if ( val )
           return val;

       return _parent && traverse ? _parent->lookup(id, traverse) : val;
   }

   /// Returns true if an ID exists.
   ///
   /// id: The ID, which may be scoped or unscoped.
   ///
   /// traverse: If true, the lookup traverses parent and child scopes as
   /// appropiate.
   ///
   bool has(shared_ptr<ID> id, bool traverse=true) const {
       auto val = find(id, traverse);
       if ( val )
           return true;

       return _parent && traverse ? _parent->has(id, traverse) : false;
   }

   // Removes a local ID from the scope.
   //
   // id: The ID to remove. It must not be scoped.
   bool remove(shared_ptr<ID> id);

   /// Dumps out a debugging representation of the scope's binding.
   ///
   /// out: The stream to use.
   void dump(std::ostream& out) const;

private:
   // Internal helpers doing the lookip opeations.
   inline Value find(shared_ptr<ID> id, bool traverse) const;
   inline Value find(typename ID::component_list::const_iterator begin, typename ID::component_list::const_iterator end, bool traverse) const;

   // Recursive version of dump().
   void dump(std::ostream& out, int level, std::set<const Scope*>* seen) const;

   shared_ptr<Scope<AstInfo>> _parent;
   scope_map _childs;
   value_map _values;
 };

template<typename AstInfo>
inline bool Scope<AstInfo>::insert(shared_ptr<ID> id, Value value)
{
    assert(! id->isScoped());

    remove(id); // Remove first if already exists.

    auto result = _values.insert(typename value_map::value_type(id->name(), value));
    return true;
}

template<typename AstInfo>
inline bool Scope<AstInfo>::remove(shared_ptr<ID> id)
{
    assert(! id->isScoped());

    auto i = _values.find(id->name());
    if ( i == _values.end() )
        return false;

    _values.erase(i);
    return true;
}

template<typename AstInfo>
inline shared_ptr<typename AstInfo::scope_value> Scope<AstInfo>::find(typename ID::component_list::const_iterator begin, typename ID::component_list::const_iterator end, bool traverse) const
{
    if ( begin == end )
        return nullptr;

    auto head = *begin;
    ++begin;

    auto val = _values.find(head);

    if ( val != _values.end() ) {
        if ( begin != end )
            // We have reached a leaf but not consumed the whole path.
            return nullptr;

        // Found.
        return val->second;
    }

    if ( ! traverse )
        return nullptr;

    auto child = _childs.find(head);

    if ( child != _childs.end() )
        // Found a child that matches our first component.
        return child->second->find(begin, end, traverse);

    // Not found.
    return nullptr;
}

template<typename AstInfo>
inline shared_ptr<typename AstInfo::scope_value> Scope<AstInfo>::find(shared_ptr<ID> id, bool traverse) const
{
    assert(traverse || ! id->isScoped());
    const typename ID::component_list& path = id->path();
    return find(path.begin(), path.end(), traverse);
}

template<typename AstInfo>
inline void Scope<AstInfo>::dump(std::ostream& out) const
{
    std::set<const Scope*> seen;
    return dump(out, 0, &seen);
}

template<typename AstInfo>
inline void Scope<AstInfo>::dump(std::ostream& out, int level, std::set<const Scope*>* seen) const
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
        out << " / " << "has parent scope " << _parent;
    else
        out << " / " << "no parent scope" << _parent;

    out << std::endl;

    for ( auto v : _values )
        out << indent << "  " << string(v.first) << " -> " << string(*v.second) << std::endl;

    for ( auto c : _childs ) {
        out << indent << "  " << c.first << std::endl;
        c.second->dump(out, level + 1, seen);
    }
}


}
#endif

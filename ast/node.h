#ifndef AST_NODE_H
#define AST_NODE_H

#include <cassert>
#include <list>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>

#include "common.h"
#include "location.h"
#include "memory.h"

namespace ast {

class NodeBase;

#ifdef DEBUG
#define __checkCast(p, _p) if ( (! p) && *_p ) { fprintf(stderr, "failed cast in ast::Node: got object of type %s\n", typeid(*((*_p).get())).name()); abort(); }
#else
#define __checkCast(p, _p)
#endif

/// A shared pointer to a Node. This pointer operates similar to a
/// shared_ptr: a node will generally be deleted once no node_ptr refers to
/// it anymore. There's a major difference though: if a node_ptr is copied,
/// the new pointer stays attached to the existing one. If either is changed
/// to point to a different Node, the change will be reflected by both
/// pointers. This allows to have multiple AST nodes storing pointers to the
/// same child node with all of them noticing when the child gets replaced
/// with something else.
///
/// node_ptr can in general by used transparently as a shared_ptr as well,
/// there are corresponding conversion operators. Note however that the tight
/// link gets lots if you first convert it to a shared_ptr and then create a
/// node_ptr from there. Always copy/assign node_ptr directly.
template<typename T>
class node_ptr
{
public:
    typedef shared_ptr<NodeBase> ptr;

    node_ptr()                         { _p = shared_ptr<ptr>(new ptr()); }
    node_ptr(std::nullptr_t)           { _p = shared_ptr<ptr>(new ptr()); }

    template<typename S>
    node_ptr(const node_ptr<S>& other) { _p = other.get_shared(); }

    template<typename S>
    node_ptr(shared_ptr<S> p)          { _p =  shared_ptr<ptr>(new ptr(p));  }

    explicit node_ptr(shared_ptr<ptr> p) { _p = p; }

    T* get() const { assert(_p); auto p = std::dynamic_pointer_cast<T>(*_p).get(); __checkCast(p, _p); return p; }

    T* operator->() const { return get(); }
    T& operator*() const  { return *get(); }

    explicit operator bool() const { return _p.get()->get() != 0; }

    // Dynamically casts the pointer to a shared_ptr of given type. Returns a
    // nullptr if the dynamic cast fails
    template<typename S>
    operator shared_ptr<S>() const { auto p = std::dynamic_pointer_cast<S>(*_p); __checkCast(p, _p); return p;}

    node_ptr<T>& operator=(const node_ptr<T>& other) { _p = other._p; return *this; }

    template<typename S>
    node_ptr<T>& operator=(const shared_ptr<S>& other) { *_p = other; return *this; }

    shared_ptr<ptr>  get_shared() const { return _p; }

    bool operator==(const node_ptr<T>& other) { return *_p == *other._p; }
    bool operator!=(const node_ptr<T>& other) { return *_p != *other._p; }

private:

    shared_ptr<ptr> _p;
};

template<typename T, typename U>
bool operator==(node_ptr<T> const& a, node_ptr<U> const& b) {
    return a.get() == b.get();
}

template<typename T, typename U>
bool operator!=(node_ptr<T> const& a, node_ptr<U> const& b) {
    return a.get() != b.get();
}

/// Dynamic check whether a node is an instance of a class derived from a given Base.
///
/// Returns: True if n is derived from class T.
template<typename T>
inline bool isA(shared_ptr<NodeBase> n) {
    return std::dynamic_pointer_cast<T>(n) != 0;
}

#if 1

/// Dynamic cast of a node pointer to a specific Node-derived class.
///
/// Deprecated. Use \a tryCast or \a checkedCast instead.
///
/// Returns: The cast pointer, or null if the dynamic cast fails.
template<typename T>
inline shared_ptr<T> as(shared_ptr<NodeBase> n) {
    return std::dynamic_pointer_cast<T>(n);
}

#endif

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version continues if the cast fails by returning null.
///
/// Returns: The cast pointer, or null if the dynamic cast fails.
template<typename T>
inline shared_ptr<T> tryCast(shared_ptr<NodeBase> n) {
    return std::dynamic_pointer_cast<T>(n);
}

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version continues if the cast fails by returning null.
///
/// Returns: The cast pointer, or null if the dynamic cast fails.
template<typename T>
inline T* tryCast(NodeBase* n) {
    return dynamic_cast<T*>(n);
}

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version aborts if the cast fails.
///
/// Returns: The cast pointer.
template<typename T>
inline shared_ptr<T> checkedCast(shared_ptr<NodeBase> n) {
    auto c = std::dynamic_pointer_cast<T>(n);

    if ( ! c ) {
        fprintf(stderr, "internal error: ast::checkedCast() failed; want '%s' but got a '%s'",
                typeid(T).name(), typeid(n.get()).name());
        abort();
    }

    return c;
}

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version aborts if the cast fails.
///
/// Returns: The cast pointer.
template<typename T>
inline T* checkedCast(NodeBase* n) {
    auto c = dynamic_cast<T*>(n);

    if ( ! c ) {
        fprintf(stderr, "internal error: ast::checkedCast() failed; want '%s' but got a '%s'",
                typeid(T).name(), typeid(n).name());
        abort();
    }

    return c;
}

/// Template-free base class for AST nodes. This class implements all
/// functionality that does not need any template parameters. Node then
/// derives from this to add what's missing.
class NodeBase : public std::enable_shared_from_this<NodeBase>
{
public:
    /// Constructor.
    ///
    /// l: A location associated with the node.
    NodeBase(const Location& l=Location::None) : _location(l) {}

    NodeBase(const NodeBase& other) {
       _location = other._location;
       _comment = other._comment;
       _childs = _childs;
    }

    virtual ~NodeBase();

    /// Returns the parents of this node. A node gets a parent once it gets
    /// added to another node via addChild().
    const std::list<NodeBase*>& parents() const { return _parents; }

    /// Returns the next sibling of the given child node. Returns null if no
    /// sibling or that's not a child.
    NodeBase* siblingOfChild(NodeBase* child) const;

    /// Returns all the parents of a given type, in the order they are
    /// encountered when going up the tree.
    template<typename T>
    std::list<shared_ptr<T>> parents() {
        std::list<shared_ptr<T>> dst;

        for ( auto p : _parents ) {
            if ( dynamic_cast<T*>(p) )
                dst.push_back(p->sharedPtr<T>());

            dst.merge(p->parents<T>());
        }

        return dst;
    }

    /// Returns the first parent of a given type, as encountered in the list
    /// returned by parents(); null if none.
    template<typename T>
    shared_ptr<T> firstParent() {
        std::list<shared_ptr<T>> p = parents<T>();
        return p.size() ? p.front() : nullptr;
    }

    /// Returns the location associated with the node, or Location::None if
    /// none.
    const Location& location() const { return _location; }

    /// Returns any comment that may be associated with the node.
    const string& comment() const { return _comment; }

    /// Associates a comment with the node.
    void setComment(const string& comment) { _comment = comment; }

    /// Dumps out a textual representation of the node, including all its
    /// childs. For each node, its render() output will be included.
    ///
    /// out: The stream to use.
    void dump(std::ostream& out);

    /// Returns a non-recursive textual representation of the node. The output
    /// of render() will be included.
    operator string();

    /// Cast operator converting into a Location.
    operator const Location&() const { return _location; }

    typedef std::list<node_ptr<NodeBase> > node_list;

    /// Returns a list of all child nodes. Child nodes are added via addChild().
    const node_list& childs() const { return _childs; }

    /// Returns an iterator to the first child node.
    node_list::const_iterator begin() const { return _childs.begin(); }

    /// Returns an iterator marking the end of the child list.
    node_list::const_iterator end() const { return _childs.end(); }

    /// Returns a shared_ptr for the node, cast to a sepcified type. The cast
    /// must not fail, behaviour is undefined if it does.
    template<typename T>
    shared_ptr<T> sharedPtr() {
       auto p = std::dynamic_pointer_cast<T>(shared_from_this());
       assert(p);
       return p;
    }

    /// Returns a shared_ptr for the node, cast to a sepcified type. The cast
    /// must not fail, behaviour is undefined if it does.
    template<typename T>
    shared_ptr<const T> sharedPtr() const {
       auto p = std::dynamic_pointer_cast<const T>(shared_from_this());
       assert(p);
       return p;
    }

    /// Adds a child node.
    ///
    /// node: The chold.
    void addChild(node_ptr<NodeBase> node);

    /// Removes a child node.
    ///
    /// node: The child to remove.
    void removeChild(node_ptr<NodeBase> node);

    /// Removes a child node.
    ///
    /// node: An iterator pointing to the child to remove.
    void removeChild(node_list::iterator node);

    /// Replaces the node with another one in all of the node's parents. Note
    /// that all corresponding node_ptr will reflect the change.
    ///
    /// n: The new node.
    void replace(shared_ptr<NodeBase> n);

    /// Returns true if a given node is a child of this one.
    bool hasChild(node_ptr<NodeBase> n) const;

    /// Internal method. Overriden by the \c ACCEPT_* macros in visitor.h.
    virtual const char* acceptClass() const { return "<acceptClass not set>"; };

    /// Returns a readable one-line representation of the node, if
    /// implemented.
    virtual string render() { return string("<node>"); }

private:
    NodeBase& operator = (const NodeBase&); // No assignment.

    typedef std::set<const NodeBase*> node_set;
    void dump(std::ostream& out, int level, node_set* seen);

    std::list<NodeBase*>  _parents;
    Location _location;
    string _comment;
    node_list _childs;
};

/// Templateized version of NodeBase. This is the class one should normally
/// derive new node types from.
template<typename AstInfo>
class Node : public NodeBase
{
public:
    typedef typename AstInfo::visitor_interface VisitorInterface;

    /// Constructor.
    ///
    /// l: A location associated with the node.
    Node(const Location& l=Location::None) : NodeBase(l) {}

    /// Visitor support. This function will be automatically overwritten when
    /// using any of the \c ACCEPT_* macros in visitor.h.
    virtual void accept(VisitorInterface *visitor) = 0;
};

}

#endif

#ifndef AST_NODE_H
#define AST_NODE_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include <set>
#include <sstream>

#include "common.h"
#include "location.h"
#include "memory.h"
#include "meta-info.h"
#include "rtti.h"

namespace ast {

class NodeBase;

#if 0

template<typename T>
inline T*ast::rtti::isA(shared_ptr<NodeBase> n) {
    return ast::rtti::isA<T>(n.get());
}

template<typename T>
inline T*ast::rtti::isA(NodeBase* n) {
    return ast::rtti::isA<T>(n);
}

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version continues if the cast fails by returning null.
///
/// Returns: The cast pointer, or null if the dynamic cast fails.
template<typename T>
inline shared_ptr<T> tryCast(shared_ptr<NodeBase> n) {
    if ( auto p = ast::rtti::tryCast<T>(n.get()) )
        return std::static_pointer_cast<T>(p->shared_from_this());
    else
        return 0;
}

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version continues if the cast fails by returning null.
///
/// Returns: The cast pointer, or null if the dynamic cast fails.
template<typename T>
inline T* tryCast(NodeBase* n) {
    return ast::rtti::tryCast<T>(n);
}

#endif

#if 0
/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version aborts if the cast fails.
///
/// Returns: The cast pointer.
template<typename T>
inline shared_ptr<T> checkedCast(shared_ptr<NodeBase> n) {
    return std::static_pointer_cast<T>(ast::rtti::checkedCast<T>(n.get())->shared_from_this());
}

/// Dynamic cast of a node pointer to a specific Node-derived class. This
/// version aborts if the cast fails.
///
/// Returns: The cast pointer.
template<typename T>
inline T* checkedCast(NodeBase* n) {
    return ast::rtti::checkedCast<T>(n)->shared_from_this();
}
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
template <typename T>
class node_ptr {
public:
    typedef shared_ptr<NodeBase> ptr;

    node_ptr()
    {
        _p = shared_ptr<ptr>(new ptr());
    }
    node_ptr(std::nullptr_t)
    {
        _p = shared_ptr<ptr>(new ptr());
    }

    template <typename S>
    node_ptr(const node_ptr<S>& other)
    {
        _assertType(other.get_shared()->get(), true);
        _p = other.get_shared();
    }

    // TODO: This should better be explcit.
    template <typename S>
    node_ptr(shared_ptr<S> p)
    {
        _assertType(p.get());
        _p = shared_ptr<ptr>(new ptr(p));
    }

    explicit node_ptr(shared_ptr<ptr> p)
    {
        _p = p;
    }

    T* get() const
    {
        return static_cast<T*>(_p->get());
    }

    T* operator->() const
    {
        return get();
    }
    T& operator*() const
    {
        return *get();
    }

    explicit operator bool() const
    {
        return _p.get()->get() != 0;
    }

    // Dynamically casts the pointer to a shared_ptr of given type. Returns a
    // nullptr if the dynamic cast fails
    template <typename S>
    operator shared_ptr<S>() const
    {
        return ast::rtti::checkedCast<S>(*_p);
    }

    node_ptr<T>& operator=(const node_ptr<T>& other)
    {
        _assertType((*other._p).get());
        _p = other._p;
        return *this;
    }

    template <typename S>
    node_ptr<T>& operator=(const shared_ptr<S>& other)
    {
        *_p = other;
        return *this;
    }

    bool operator==(const node_ptr<T>& other)
    {
        return *_p == *other._p;
    }
    bool operator!=(const node_ptr<T>& other)
    {
        return *_p != *other._p;
    }

    // For ther instances of this template to access the internal state.
    shared_ptr<ptr> get_shared() const
    {
        return _p;
    }

private:
    static void _assertType(NodeBase* p, bool ok_on_init = false)
    {
        if ( ! p )
            return;

        if ( ! ast::rtti::isA<T>(p, ok_on_init) ) {
            fprintf(stderr,
                    "internal error: ast::node_ptr type mismatch; wanted '%s' but got a '%s'\n",
                    ast::rtti::typeName<T>(), ast::rtti::typeName(p));
            abort();
        }
    }

    shared_ptr<ptr> _p;
};

template <typename T, typename U>
bool operator==(node_ptr<T> const& a, node_ptr<U> const& b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
bool operator!=(node_ptr<T> const& a, node_ptr<U> const& b)
{
    return a.get() != b.get();
}

#if 0
namespace rtti {

// Support RTTI casting from node_ptr. Note we return only plain pointers
// here.
template<typename T>
inline ast::rtti::RTTI::ID typeId(node_ptr<T> t)
{
    return t->rtti().id();
}

template<typename D>
D* tryCast(node_ptr<NodeBase> n)
{
    return tryCast<D>(n.get());
}

template<typename D>
D* checkedCast(node_ptr<NodeBase> n)
{
    return checkedCast<D>(n.get());
}

}
#endif

/// Template-free base class for AST nodes. This class implements all
/// functionality that does not need any template parameters. Node then
/// derives from this to add what's missing.
class NodeBase : public std::enable_shared_from_this<NodeBase>, virtual public ast::rtti::Base {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// l: A location associated with the node.
    NodeBase(const Location& l = Location::None) : _location(l)
    {
    }

    NodeBase(const NodeBase& other)
    {
        _location = other._location;
        _comments = other._comments;
        _childs = other._childs;
    }

    virtual ~NodeBase();

    /// Returns the parents of this node. A node gets a parent once it gets
    /// added to another node via addChild().
    const std::list<NodeBase*>& parents() const
    {
        return _parents;
    }

    /// Returns the next sibling of the given child node. Returns null if no
    /// sibling or that's not a child.
    NodeBase* siblingOfChild(NodeBase* child) const;

    /// Returns all the parents of a given type, in the order they are
    /// encountered when going up the tree.
    template <typename T>
    void _parentsInternal(NodeBase* n, std::list<shared_ptr<T>>* dst, std::set<NodeBase*>* done,
                          int level = 0)
    {
        for ( auto p : n->_parents ) {
            if ( done->find(p) != done->end() )
                continue;

            done->insert(p);

            if ( ast::rtti::isA<T>(p) )
                dst->push_back(p->template sharedPtr<T>());

            _parentsInternal(p, dst, done, level + 1);
        }
    }

    /// Returns all the parents of a given type, in the order they are
    /// encountered when going up the tree.
    template <typename T>
    std::list<shared_ptr<T>> parents()
    {
        std::list<shared_ptr<T>> dst;
        std::set<NodeBase*> done;
        _parentsInternal(this, &dst, &done);
        return dst;
    }

    /// Returns the first parent of a given type, as encountered in the list
    /// returned by parents(); null if none.
    ///
    /// Deprecated: This function does not produce well-defined results, as
    /// it depends on the order in which the parents happen to be added. We
    /// should remove the method and adapt the current callers to get the
    /// node the need in somer other way.
    ///
    /// In fact, we should try to remove the whole parent pointering; it's
    /// complex, may still have a bug, and may not be necessart anyways.
    template <typename T>
    shared_ptr<T> firstParent()
    {
        std::list<shared_ptr<T>> p = parents<T>();
        return p.size() ? p.front() : nullptr;
    }

    /// Returns the location associated with the node, or Location::None if
    /// none.
    const Location& location() const
    {
        return _location;
    }

    /// Returns any comment that may be associated with the node.
    const std::list<string>& comments() const
    {
        return _comments;
    }

    /// Associates a comment with the node.
    void addComment(const string& comment)
    {
        _comments.push_back(comment);
    }

    /// Dumps out a textual representation of the node, including all its
    /// childs. For each node, its render() output will be included.
    ///
    /// out: The stream to use.
    void dump(std::ostream& out);

    /// Returns a non-recursive textual representation of the node. The output
    /// of render() will be included.
    operator string();

    /// Cast operator converting into a Location.
    operator const Location&() const
    {
        return _location;
    }

    typedef std::list<node_ptr<NodeBase>> node_list;

    /// Returns a list of all child nodes, recursively if requestd. Child
    /// nodes are added via addChild().
    const node_list childs(bool recursive = false) const;

    /// Returns an iterator to the first child node.
    node_list::const_iterator begin() const
    {
        return _childs.begin();
    }

    /// Returns an iterator marking the end of the child list.
    node_list::const_iterator end() const
    {
        return _childs.end();
    }

    /// Returns a reverse iterator to the lastt child node.
    node_list::const_reverse_iterator rbegin() const
    {
        return _childs.rbegin();
    }

    /// Returns a reverse iterator marking the end of the reversed child list.
    node_list::const_reverse_iterator rend() const
    {
        return _childs.rend();
    }

    /// Returns a shared_ptr for the node, cast to a sepcified type. The cast
    /// must not fail, behaviour is undefined if it does.
    template <typename T>
    shared_ptr<T> sharedPtr()
    {
        return ast::rtti::checkedCast<T>(shared_from_this());
    }

    /// Returns a shared_ptr for the node, cast to a sepcified type. The cast
    /// must not fail, behaviour is undefined if it does.
    template <typename T>
    shared_ptr<const T> sharedPtr() const
    {
        return ast::rtti::checkedCast<const T>(shared_from_this());
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

    /// Removes the node from all its parents.
    void removeFromParents();

    /// Replaces the node with another one in all of the node's parents. Note
    /// that all corresponding node_ptr will reflect the change.
    ///
    /// n: The new node.
    ///
    /// parent: If given, replace only for this parent.
    void replace(shared_ptr<NodeBase> n, shared_ptr<NodeBase> parent = nullptr);

    /// Disconnects the node from all its childs and parents.
    void disconnect();

    /// Returns true if a given node is a child of this one.
    ///
    /// recursive: If true, descend the tree recursively for seaching the
    /// child
    bool hasChild(node_ptr<NodeBase> n, bool recursive = false) const;

    /// Returns true if a given node is a child of this one.
    ///
    /// recursive: If true, descend the tree recursively for seaching the
    /// child
    bool hasChild(NodeBase* n, bool recursive = false) const;

    /// Returns the set of meta information nodes associated with the AST
    /// node.
    MetaInfo* metaInfo();

    /// Internal method. Overriden by the \c ACCEPT_* macros in visitor.h.
    virtual const char* acceptClass() const
    {
        return "<acceptClass not set>";
    };

    /// Returns a readable one-line representation of the node, if
    /// implemented.
    virtual string render()
    {
        return string("<node>");
    }

private:
    typedef std::set<NodeBase*> node_set;

    NodeBase& operator=(const NodeBase&); // No assignment.

    bool hasChildInternal(NodeBase* node, bool recursive, node_set* done) const;
    void childsInternal(const NodeBase* node, bool recursive, node_set* childs) const;
    void dump(std::ostream& out, int level, node_set* seen);

    std::list<NodeBase*> _parents;
    node_list _childs;

    Location _location;
    std::list<string> _comments;
    MetaInfo _meta;
};

namespace rtti {

template <typename T>
inline RTTI::ID typeId(node_ptr<T> t)
{
    return t->rtti().id();
}
}

/// Templateized version of NodeBase. This is the class one should normally
/// derive new node types from.
template <typename AstInfo>
class Node : public NodeBase {
public:
    typedef typename AstInfo::visitor_interface VisitorInterface;

    /// Constructor.
    ///
    /// l: A location associated with the node.
    Node(const Location& l = Location::None) : NodeBase(l)
    {
    }

    /// Visitor support. This function will be automatically overwritten when
    /// using any of the \c ACCEPT_* macros in visitor.h.
    virtual void accept(VisitorInterface* visitor) = 0;
};
}

#endif

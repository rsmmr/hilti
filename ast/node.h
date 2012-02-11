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

   template<typename S>
   node_ptr(const node_ptr<S>& other) { _p = other.get_shared(); }

   template<typename S>
   node_ptr(shared_ptr<S> p)          { _p =  shared_ptr<ptr>(new ptr(p));  }

   explicit node_ptr(shared_ptr<ptr> p) { _p = p; }

   T* get() const { assert(_p); auto p = std::dynamic_pointer_cast<T>(*_p).get(); assert(p || !*_p); return p; }

   T* operator->() const { return get(); }
   T& operator*() const  { return *get(); }

   operator bool() const { return *_p; }

   // Dynamically casts the pointer to a shared_ptr of given type. Returns a
   // nullptr if the dynamic cast fails
   template<typename S>
   operator shared_ptr<S>() const { auto p = std::dynamic_pointer_cast<S>(*_p); assert(p || !*_p); return p;}

   node_ptr<T>& operator=(const node_ptr<T>& other) { _p = other._p; return *this; }

   template<typename S>
   node_ptr<T>& operator=(const shared_ptr<S>& other) { *_p = other; return *this; }

   shared_ptr<ptr>  get_shared() const { return _p; }

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

/// Dynamic cast of a node pointer to a specific Node-derived class.
///
/// Returns: The cast pointer, or null if the dynamic cast fails.
template<typename T>
inline shared_ptr<T> as(shared_ptr<NodeBase> n) {
    return std::dynamic_pointer_cast<T>(n);
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
   NodeBase(const Location& l=Location::None) : _parent(0), _location(l) {}

   NodeBase(const NodeBase& other) {
       _parent = 0;
       _location = other._location;
       _comment = other._comment;
       _childs = _childs;
   }

   virtual ~NodeBase() {}

   /// Returns the parent to this node. A node gets a parent once it gets
   /// added to another node via addChild(). If a node gets added to multiple
   /// parents, the last one will be returned.
   NodeBase* parent() const { return _parent; }

   /// Returns the closest parent of a given type.
   template<typename T>
   shared_ptr<T> parent() {
       if ( ! _parent )
           return shared_ptr<T>();

       if ( dynamic_cast<T*>(_parent) )
           return _parent->sharedPtr<T>();

       return _parent->parent<T>();
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
   void dump(std::ostream& out) const;

   /// Returns a non-recursive textual representation of the node. The output
   /// of render() will be included.
   operator string() const;

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

   /// Replaces a child node with another one. Node that all node_ptr to this
   /// child will reflect the update.
   ///
   /// old_node: The old child node.
   ///
   /// new_node: The new child node.
   void replaceChild(shared_ptr<NodeBase> old_node, shared_ptr<NodeBase> new_node);

   /// Internal method. Overriden by the \c ACCEPT_* macros in visitor.h.
   virtual const char* acceptClass() const { return "<acceptClass not set>"; };

   /// Method derived classes can overwrite to add custom output to #string
   /// and #dump.
   virtual void render(std::ostream& out) const { }

private:
   NodeBase& operator = (const NodeBase&); // No assignment.

   typedef std::set<const NodeBase*> node_set;
   void dump(std::ostream& out, int level, node_set* seen) const;

   NodeBase* _parent;
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

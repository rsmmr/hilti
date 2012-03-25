
#include <util/util.h>

#include "node.h"

using namespace ast;

void NodeBase::addChild(node_ptr<NodeBase> node) {
    if ( ! node )
        return;

#if 0
    // Make sure we don't get loop.
    for ( auto p = this; p; p = p->_parent ) {
        if ( p == node.get() ) {
            fprintf(stderr, "internal error in addChild(): recursive node detected\n");
            abort();
        }
    }
#endif

    _childs.push_back(node);

    auto n = dynamic_cast<NodeBase*>(node.get());
    n->_parent = this;
}

void NodeBase::removeChild(node_ptr<NodeBase> node) {
    _childs.remove(node);
    auto n = dynamic_cast<NodeBase*>(node.get());
    n->_parent = 0;
}

void NodeBase::removeChild(node_list::iterator node) {
    _childs.erase(node);
    auto n = dynamic_cast<NodeBase*>((*node).get());
    assert(n->_parent == this);
    n->_parent = 0;
}

void NodeBase::replaceChild(shared_ptr<NodeBase> old_node, shared_ptr<NodeBase> new_node) {
    shared_ptr<NodeBase> o = std::dynamic_pointer_cast<NodeBase>(old_node);
    shared_ptr<NodeBase> n = std::dynamic_pointer_cast<NodeBase>(new_node);
    assert(o->_parent == this);
    o->_parent = 0;
    n->_parent = this;

    for ( auto c : _childs ) {
        if ( c.get() == old_node.get() )
            c = n;
    }
}

NodeBase::operator string() {

    string s = render();
    string location = "-";

    if ( _location )
        location = string(_location);

    return util::fmt("%s [%d/%s %p] %s", typeid(*this).name(),
                     _childs.size(), location.c_str(), this, s.c_str());
}

void NodeBase::dump(std::ostream& out)
{
    node_set seen;
    dump(out, 0, &seen);
}

void NodeBase::dump(std::ostream& out, int level, node_set* seen)
{
    for ( int i = 0; i < level; ++i )
        out << "  ";

    if ( seen->find(this) == seen->end() ) {
        seen->insert(this);

        out << string(*this) << std::endl;

        for ( auto c : _childs )
            c->dump(out, level + 1, seen);
    }

    else
        out << string(*this) << " (childs suppressed due to recursion)" << std::endl;
}

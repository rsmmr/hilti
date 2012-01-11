
#include <util/util.h>

#include "node.h"

using namespace ast;

void NodeBase::addChild(node_ptr<NodeBase> node) {
       if ( ! node )
        return;

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

NodeBase::operator string() const {

    std::ostringstream sout;
    render(sout);

    const char* location = _location ? string(_location).c_str() : "-";

    return util::fmt("%s [%d/%s %p] %s", typeid(*this).name(),
                     _childs.size(), location, this, sout.str().c_str());
}

void NodeBase::dump(std::ostream& out, int level) const
{
    for ( int i = 0; i < level; ++i )
        out << "  ";

    out << string(*this) << std::endl;

    for ( auto c : _childs )
        c->dump(out, level + 1);
}

#include <stdlib.h>
#include <cxxabi.h>

#include <util/util.h>

#include "node.h"

using namespace ast;

NodeBase::~NodeBase()
{
    for ( auto c : _childs )
        c->_parents.remove(this);

#if 0
    // This crashes, the sharedPtr() call?
    for ( auto p : _parents )
        p->_childs.remove(this->sharedPtr<NodeBase>());
#endif
}

void NodeBase::addChild(node_ptr<NodeBase> node)
{
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
    node->_parents.push_back(this);
}

bool NodeBase::hasChildInternal(node_ptr<NodeBase> node, bool recursive, std::set<shared_ptr<NodeBase>>* done) const
{
    for ( auto c: _childs ) {

        if ( done->find(c) != done->end() )
            continue;

        done->insert(c);

        if ( c.get() == node.get() )
            return true;

        if ( recursive && c->hasChildInternal(node, true, done) )
            return true;
    }

    return false;
}

bool NodeBase::hasChild(node_ptr<NodeBase> node, bool recursive) const
{
    std::set<shared_ptr<NodeBase>> done;
    return hasChildInternal(node, recursive, &done);
}

NodeBase::node_list NodeBase::childs(bool recursive) const
{
    if ( ! recursive )
        return _childs;

    std::set<shared_ptr<NodeBase>> childs;
    childsInternal(this, recursive, &childs);

    node_list result;
    for ( auto c : childs )
        result.push_back(c);

    return result;
}

void NodeBase::childsInternal(const NodeBase* node, bool recursive, std::set<shared_ptr<NodeBase>>* childs) const
{
    for ( auto c : node->_childs ) {
        if ( childs->find(c) != childs->end() )
            continue;

        childs->insert(c);
        childsInternal(c.get(), recursive, childs);
    }
}

void NodeBase::removeChild(node_ptr<NodeBase> node)
{
    assert(hasChild(node));

    _childs.remove(node);
    node->_parents.remove(this);
}

void NodeBase::removeChild(node_list::iterator node)
{
    assert(hasChild(*node));

    _childs.remove(*node);
    (*node)->_parents.remove(this);
}

void NodeBase::removeFromParents()
{
    for ( auto p : _parents )
        p->_childs.remove(this->sharedPtr<NodeBase>());

    _parents.clear();
}

void NodeBase::replace(shared_ptr<NodeBase> n)
{
    std::list<std::pair<shared_ptr<NodeBase>, NodeBase*>> add_parents;
    std::list<std::pair<shared_ptr<NodeBase>, NodeBase*>> del_parents;
//        for ( node_list::iterator i = p->_childs.begin(); i != p->_childs.end(); i++ ) {
//            auto c = (*i).get();

    for ( auto p : _parents ) {
        for ( auto c : p->_childs ) {
            if ( c.get() == this ) {
                c = n;
                add_parents.push_back(std::make_pair(n, p));
                // del_parents.push_back(std::make_pair(c, p));
            }
        }
    }

    for ( auto np : add_parents )
        np.first->_parents.push_back(np.second);

    for ( auto np : del_parents )
        np.first->_parents.remove(np.second);

}

NodeBase* NodeBase::siblingOfChild(NodeBase* child) const
{
    for ( auto i = _childs.begin(); i != _childs.end(); i++ ) {
        if ( (*i).get() == child ) {
            ++i;
            return i != _childs.end() ? (*i).get() : nullptr;
        }
    }

    return nullptr;
}

NodeBase::operator string() {

    string s = render();
    string location = "-";

    if ( _location )
        location = string(_location);

    int status;
    char *name = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);

    string parents = "";

    for ( auto p : _parents )
        parents += util::fmt(" p:%p", p);

    return util::fmt("%s [%d/%s %p%s] %s", name,
                     _childs.size(), location.c_str(), this, parents.c_str(), s.c_str());
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

MetaInfo* NodeBase::metaInfo()
{
    return &_meta;
}

#include <cxxabi.h>
#include <stdlib.h>

#include <util/util.h>

#include "node.h"

namespace hilti {
namespace statement {
namespace instruction {
class Unresolved;
}
}
};

using namespace ast;

AST_RTTI_BEGIN(NodeBase, ast_NodeBase)
AST_RTTI_END(ast_NodeBase)

AST_RTTI_CAST_BEGIN(NodeBase)
AST_RTTI_CAST_END(NodeBase)

NodeBase::~NodeBase()
{
    for ( auto c : _childs )
        c->_parents.remove(this);

    bool changed = false;

    do {
        changed = false;

        for ( auto p : _parents ) {
            for ( auto c = p->_childs.begin(); c != p->_childs.end(); c++ ) {
                if ( (*c).get() == this ) {
                    p->_childs.erase(c);
                    changed = true;
                    break;
                }
            }

            if ( changed )
                break;
        }
    } while ( changed );

    _parents.clear();
}

void NodeBase::addChild(node_ptr<NodeBase> node)
{
    if ( ! node.get() )
        return;

    bool exists = false;

    for ( auto i = _childs.begin(); i != _childs.end(); i++ ) {
        if ( &(*i) == &node )
            return;

        if ( (*i).get() == node.get() )
            exists = true;
    }

    assert(node);
    _childs.push_back(node);
    node->_parents.push_back(this);
}

void NodeBase::removeChild(node_ptr<NodeBase> node)
{
    if ( ! node )
        return;

    bool changed = false;

    do {
        changed = false;

        for ( auto i = _childs.begin(); i != _childs.end(); i++ ) {
            if ( (*i).get() == node.get() ) {
                bool found = false;

                for ( auto j = node->_parents.begin(); j != node->_parents.end(); j++ ) {
                    if ( *j == this ) {
                        found = true;
                        node->_parents.erase(j);
                        break;
                    }
                }

                assert(found);

                _childs.erase(i);
                changed = true;
                break;
            }
        }

    } while ( changed );
}

void NodeBase::removeChild(node_list::iterator i)
{
    auto node = *i;

    bool found = false;

    for ( auto j = node->_parents.begin(); j != node->_parents.end(); j++ ) {
        if ( *j == this ) {
            found = true;
            node->_parents.erase(j);
            break;
        }
    }

    assert(found);

    _childs.erase(i);
}

void NodeBase::removeFromParents()
{
    bool changed = false;

    do {
        changed = false;

        for ( auto p : _parents ) {
            for ( auto c = p->_childs.begin(); c != p->_childs.end(); c++ ) {
                if ( (*c).get() == this ) {
                    p->removeChild(c);
                    changed = true;
                    break;
                }
            }

            if ( changed )
                break;
        }
    } while ( changed );

    assert(_parents.size() == 0);
}

void NodeBase::replace(shared_ptr<NodeBase> n, shared_ptr<NodeBase> parent)
{
    assert(n);

    if ( n.get() == this )
        return;

    std::list<NodeBase*> parents = _parents;

    bool changed;

    do {
        changed = false;

        for ( auto p : _parents ) {
            if ( parent && parent.get() != p )
                continue;

            for ( auto c = p->_childs.begin(); c != p->_childs.end(); c++ ) {
                if ( (*c).get() == this ) {
                    node_ptr<NodeBase> old = *c;
                    p->addChild(n);
                    p->removeChild(c);
                    old = n;
                    changed = true;
                    goto next;
                }
            }
        }
    next : {
    }
    } while ( changed );

    if ( ! parent ) {
        for ( auto c : _childs )
            c->_parents.remove(this);

        _childs.clear();
    }
}


bool NodeBase::hasChildInternal(NodeBase* node, bool recursive, node_set* done) const
{
    assert(done || ! recursive);

    for ( auto c : _childs ) {
        if ( done ) {
            if ( done->find(c.get()) != done->end() )
                continue;

            done->insert(c.get());
        }

        if ( c.get() == node )
            return true;

        if ( recursive && c->hasChildInternal(node, true, done) )
            return true;
    }

    return false;
}

bool NodeBase::hasChild(node_ptr<NodeBase> node, bool recursive) const
{
    return hasChild(node.get(), recursive);
}

bool NodeBase::hasChild(NodeBase* node, bool recursive) const
{
    node_set done;
    return hasChildInternal(node, recursive, &done);
}

const NodeBase::node_list NodeBase::childs(bool recursive) const
{
    if ( ! recursive )
        return _childs;

    node_set childs;
    childsInternal(this, recursive, &childs);

    node_list result;
    for ( auto c : childs )
        result.push_back(c->shared_from_this());

    return result;
}

void NodeBase::childsInternal(const NodeBase* node, bool recursive, node_set* childs) const
{
    for ( auto c : node->_childs ) {
        if ( childs->find(c.get()) != childs->end() )
            continue;

        childs->insert(c.get());
        childsInternal(c.get(), recursive, childs);
    }
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

NodeBase::operator string()
{
    string s = render();
    string location = "-";

    if ( _location )
        location = string(_location);

    const char* name =
        "CURRENTLY DISABLED, TODO"; // abi::__cxa_demangle(typeName(*this), 0, 0, &status);

    string parents = "";

    for ( auto p : _parents )
        parents += util::fmt(" p:%p", p);

    return util::fmt("%s [%d/%s %p%s] %s", name, _childs.size(), location.c_str(), this,
                     parents.c_str(), s.c_str());
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

        for ( auto c : _childs ) {
            if ( c )
                c->dump(out, level + 1, seen);

            else {
                for ( int i = 0; i < level + 1; ++i )
                    out << "  ";
                out << "NULL\n";
            }
        }
    }

    else
        out << string(*this) << " (childs suppressed due to recursion)" << std::endl;
}

MetaInfo* NodeBase::metaInfo()
{
    return &_meta;
}

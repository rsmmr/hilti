
#include <assert.h>

#include "meta-info.h"

using namespace ast;

MetaNode::MetaNode(const string& name)
{
    _name = name;
}


MetaNode::~MetaNode()
{
}

const string& MetaNode::name() const
{
    return _name;
}

string MetaNode::render() const
{
    return "%" + _name;
}

MetaInfo::MetaInfo()
{
}

MetaInfo::~MetaInfo()
{
}

bool MetaInfo::has(const string& key)
{
    return _nodes.find(key) != _nodes.end();
}

shared_ptr<MetaNode> MetaInfo::lookup(const string& key) const
{
    auto i = _nodes.find(key);
    return i != _nodes.end() ? (*i).second : nullptr;
}

std::list<shared_ptr<MetaNode>> MetaInfo::lookupAll(const string& key) const
{
    std::list<shared_ptr<MetaNode>> nodes;

    auto m = _nodes.equal_range(key);

    for ( auto n = m.first; n != m.second; ++n )
        nodes.push_back((*n).second);

    return nodes;
}

void MetaInfo::add(shared_ptr<MetaNode> n)
{
    _nodes.insert(std::make_pair(n->name(), n));
}

void MetaInfo::remove(const string& key)
{
    _nodes.erase(key);
}

void MetaInfo::remove(shared_ptr<MetaNode> n)
{
    for ( auto i = _nodes.begin(); i != _nodes.end(); i++ ) {
        if ( (*i).second == n ) {
            _nodes.erase(i);
            return;
        }
    }

    // Node not found.
    assert(false);
}

MetaInfo::operator string() const
{
    string s;

    for ( auto i = _nodes.begin(); i != _nodes.end(); i++ ) {
        if ( s.size() )
            s += " ";

        s += (*i).second->render();
    }

    return s;
}

size_t MetaInfo::size() const
{
    return _nodes.size();
}

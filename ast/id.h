
#ifndef AST_ID_H
#define AST_ID_H

#include <cassert>

#include <util/util.h>

#include "node.h"
#include "visitor.h"

namespace ast {

/// An AST node for A source-level identifier. Identifiers can be either
/// scoped (\c Foo::bar) or non-scoped (\c bar).
template <typename AstInfo>
class ID : public AstInfo::node {
public:
    typedef typename AstInfo::node Node;
    typedef typename AstInfo::id AstID;

    /// A list of the individual components of a scoped identifiers.
    typedef std::list<string> component_list;

    /// Constructor.
    ///
    /// path: The name of the identifier, either scoped or unscoped.
    ///
    /// l: Associated location.
    ID(string path, const Location& l = Location::None) : Node(l)
    {
        // FIXME: Don't understand why directly assigning to _path doesn't work.
        _path = util::strsplit(path, "::");
        assert(_path.size() > 0);
    }

    /// Constructor.
    ///
    /// path: Scope components.
    ///
    /// l: Associated location.
    ID(component_list path, const Location& l = Location::None) : Node(l)
    {
        _path = path;
        assert(_path.size() > 0);
    }

    shared_ptr<AstID> clone() const
    {
        return std::make_shared<AstID>(_path, this->location());
    }

    /// Returns the identifier as a list of components making it up. For
    /// non-scoped IDs, the returned list will have exactly one element.
    component_list path() const
    {
        return _path;
    }

    /// Returns the identifier as single string. For scoped identifiers, the
    /// path components will be joined by \c ::.
    ///
    /// relative_to: If given and the full names starts with a initial
    /// component like it, then that one is removed. Typically, one would
    /// pass in a module name here and the method then returns the ID as to
    /// be used from inside that module.
    string pathAsString(shared_ptr<ID> relative_to = nullptr) const
    {
        auto p1 = _path;
        auto p2 = relative_to ? relative_to->_path : component_list();

        while ( p1.size() && p2.size() && p1.front() == p2.front() ) {
            p1.pop_front();
            p2.pop_front();
        }

        return util::strjoin(p1, "::");
    }

    /// Returns the local component of the identifier. For \c Foo::bar this
    /// returns \c bar (as it does for \c bar).
    string name() const
    {
        return _path.back();
    }

    /// XXX
    void setOriginal(shared_ptr<AstID> org)
    {
        this->removeChild(_org);
        _org = org;
        this->addChild(_org);
    }

    /// XXX
    shared_ptr<AstID> original() const
    {
        return _org;
    }

    /// Returns the local component of the identifier. For \c Foo::bar this
    /// returns \c bar (as it does for \c bar).
    ///
    /// \note: This is currently the same as name(), but will replace it
    /// eventually; name() will then return pathAsString().
    string local() const
    {
        return _path.back();
    }

    /// Returns the scope of the ID. If it's not scoped, that's the empty string.
    string scope() const
    {
        component_list p = _path;
        p.pop_back();
        return util::strjoin(p, "::");
    }

    /// Returns true if the identifier has a scope.
    bool isScoped() const
    {
        return _path.size() > 1;
    }

    string render() /* override */
    {
        return pathAsString();
    }

    bool operator==(const ID& other) const
    {
        return pathAsString() == other.pathAsString();
    }

    bool operator==(const string& other) const
    {
        return pathAsString() == other;
    }

private:
    component_list _path;
    node_ptr<AstID> _org = nullptr;
};
}

#endif

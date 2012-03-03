
#ifndef AST_ID_H
#define AST_ID_H

#include <cassert>

#include <util/util.h>

#include "node.h"
#include "visitor.h"

namespace ast {

/// An AST node for A source-level identifier. Identifiers can be either
/// scoped (\c Foo::bar) or non-scoped (\c bar).
template<typename AstInfo>
class ID : public AstInfo::node
{
public:
   typedef typename AstInfo::node Node;

   /// A list of the individual components of a scoped identifiers.
   typedef std::list<string> component_list;

   /// Constructor.
   ///
   /// path: The name of the identifier, either scoped or unscoped.
   ///
   /// l: Associated location.
   ID(string path, const Location& l=Location::None) : Node(l) {
       // FIXME: Don't understand why directly assigning to _path doesn't work.
       _path = util::strsplit(path, "::");
       assert(_path.size() > 0);
   }

   /// Constructor.
   ///
   /// path: Scope components.
   ///
   /// l: Associated location.
   ID(component_list path, const Location& l=Location::None) : Node(l) {
       _path = path;
       assert(_path.size() > 0);
   }

   /// Returns the identifier as a list of components making it up. For
   /// non-scoped IDs, the returned list will have exactly one element.
   component_list path() const {
       return _path;
       }

   /// Returns the identifier as single string. For scoped identifiers, the
   /// path components will be joined by \c ::.
   string pathAsString() const {
       return util::strjoin(_path, "::");
       }

   /// Returns the local component of the identifier. For \c Foo::bar this
   /// returns \c bar (as it does for \c bar).
   string name() const {
       return _path.back();
   }

   /// Returns the scope of the ID. If it's not scoped, that's the empty string.
   string scope() const {
       component_list p = _path;
       p.pop_back();
       return util::strjoin(p, "::");
   }

   /// Returns true if the identifier has a scope.
   bool isScoped() const {
       return _path.size() > 1;
   }

   void render(std::ostream& out) /* override */ {
       out << pathAsString();
   }

   bool operator==(const ID& other) const {
       return pathAsString() == other.pathAsString();
   }

   bool operator==(const string& other) const {
       return pathAsString() == other;
   }

private:
   component_list _path;
};


}

#endif

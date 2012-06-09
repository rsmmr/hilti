
#ifndef HILTI_SCOPE_H
#define HILTI_SCOPE_H

#include "common.h"

namespace hilti {

/// A HILTI identifier scope. See ast::Scope.
class Scope : public ast::Scope<AstInfo> {
public:
   /// Constructor.
   ///
   /// parent: A parent scope. If an ID is not found in the current scope,
   /// lookup() will forward recursively to parent scopes.
   Scope(shared_ptr<Scope> parent = nullptr)
       : ast::Scope<AstInfo>(parent) {}
};

}

#endif

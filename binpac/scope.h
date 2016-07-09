///
/// An identifier scope.
///

#ifndef BINPAC_SCOPE_H
#define BINPAC_SCOPE_H

#include <ast/scope.h>

#include "id.h"
#include "scope.h"

namespace binpac {

/// A BinPAC++ identifier scope. See ast::Scope.
class Scope : public ast::Scope<AstInfo> {
public:
    /// Constructor.
    ///
    /// parent: A parent scope. If an ID is not found in the current scope,
    /// lookup() will forward recursively to parent scopes.
    Scope(shared_ptr<Scope> parent = nullptr);
};
}

#endif

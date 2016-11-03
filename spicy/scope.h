///
/// An identifier scope.
///

#ifndef SPICY_SCOPE_H
#define SPICY_SCOPE_H

#include <ast/scope.h>

#include "id.h"
#include "scope.h"

namespace spicy {

/// A Spicy identifier scope. See ast::Scope.
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

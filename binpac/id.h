
#ifndef BINPAC_ID_H
#define BINPAC_ID_H

#include <ast/id.h>
#include <ast/visitor.h>

#include "common.h"

namespace binpac {

/// AST node for a source-level identifier.
class ID : public ast::ID<AstInfo> {
public:
    /// Constructor.
    ///
    /// path: The name of the identifier, either scoped or unscoped.
    ///
    /// l: Associated location.
    ID(string path, const Location& l = Location::None);

    /// Constructor.
    ///
    /// path: Scope components.
    ///
    /// l: Associated location.
    ID(component_list path, const Location& l = Location::None);

    ACCEPT_VISITOR_ROOT();

private:
};
}

#endif


#ifndef SPICY_ID_H
#define SPICY_ID_H

#include <ast/id.h>
#include <ast/visitor.h>

#include "common.h"

namespace spicy {

/// AST node for a source-level identifier.
class ID : public ast::ID<AstInfo> {
    AST_RTTI
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

    /// Destructor.
    virtual ~ID();

    ACCEPT_VISITOR_ROOT();

private:
};
}

#endif

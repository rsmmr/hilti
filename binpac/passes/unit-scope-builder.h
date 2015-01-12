
#ifndef BINPAC_PASSES_UNIT_SCOPE_BUILDER_H
#define BINPAC_PASSES_UNIT_SCOPE_BUILDER_H

#include <ast/pass.h>

#include "../common.h"
#include "../ast-info.h"

namespace binpac {

class CompilerContext;

namespace passes {

/// Populates the scopes inside units. This can only run after IDs have been
/// resolved and hence it split into a separate pass.
class UnitScopeBuilder : public ast::Pass<AstInfo>
{
public:
    /// Constructor.
    ///
    /// context: The context the AST is part of.
    UnitScopeBuilder();
    virtual ~UnitScopeBuilder();

    /// Populates an AST's units.
    ///
    /// module: The AST to process.
    ///
    /// Returns: True if no error were encountered.
    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(declaration::Type* t) override;
};

}

}

#endif

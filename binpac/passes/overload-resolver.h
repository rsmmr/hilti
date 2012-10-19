
#ifndef BINPAC_PASSES_OVERLOAD_RESOLVER_H
#define BINPAC_PASSES_OVERLOAD_RESOLVER_H

#include <ast/pass.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {
namespace passes {

/// Resolves overloaded function calls. It replaces nodes of type ID with the
/// function that it's referencing.
class OverloadResolver : public ast::Pass<AstInfo>
{
public:
    OverloadResolver();

    virtual ~OverloadResolver();

    /// Resolves function overloads.
    ///
    /// module: The AST to resolve.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(expression::UnresolvedOperator* o) override;
};

}
}

#endif

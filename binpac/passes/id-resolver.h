
#ifndef BINPAC_PASSES_ID_RESOLVER_H
#define BINPAC_PASSES_ID_RESOLVER_H

#include <ast/pass.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {
namespace passes {

/// Resolves ID references. It replaces nodes of type ID with the node that
/// it's referencing.
class IDResolver : public ast::Pass<AstInfo>
{
public:
    IDResolver();

    virtual ~IDResolver();

    /// Resolves ID references.
    ///
    /// module: The AST to resolve.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(expression::ID* i) override;
    void visit(type::Unknown* t) override;
    void visit(declaration::Hook* t) override;
    void visit(type::unit::item::field::Unknown* f) override;

private:
    std::set<string> _locals;
};

}
}

#endif

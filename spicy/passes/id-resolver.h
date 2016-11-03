
#ifndef SPICY_PASSES_ID_RESOLVER_H
#define SPICY_PASSES_ID_RESOLVER_H

#include <ast/pass.h>

#include "../ast-info.h"
#include "../common.h"

namespace spicy {
namespace passes {

/// Resolves ID references. It replaces nodes of type ID with the node that
/// it's referencing.
class IDResolver : public ast::Pass<AstInfo> {
public:
    /// Constructor.
    IDResolver();

    virtual ~IDResolver();

    /// Resolves ID references.
    ///
    /// module: The AST to resolve.
    ///
    /// report_unresolved: If true, IDs that we can't resolve are reported as
    /// errorr; if false, they are simply left untouched.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<ast::NodeBase> ast, bool report_unresolved);

protected:
    void visit(expression::ID* i) override;
    void visit(expression::ListComprehension* c) override;
    void visit(type::Unknown* t) override;
    void visit(declaration::Hook* t) override;
    void visit(type::unit::item::field::Unknown* f) override;

private:
    bool run(shared_ptr<ast::NodeBase> ast) override;

    bool _report_unresolved = true;
    std::set<string> _locals;
};
}
}

#endif

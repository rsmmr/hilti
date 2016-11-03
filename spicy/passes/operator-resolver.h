
#ifndef SPICY_PASSES_OPERATOR_RESOLVER_H
#define SPICY_PASSES_OPERATOR_RESOLVER_H

#include <ast/pass.h>

#include "../ast-info.h"
#include "../common.h"

namespace spicy {
namespace passes {

/// Resolves operators. Initially, all operator nodes are created as
/// instances of expression::UnresolvedOperator. This pass turns them into
/// instances of classes derived expression::ResolvedOperator, of which we
/// have one class per operator type.
class OperatorResolver : public ast::Pass<AstInfo> {
public:
    OperatorResolver(shared_ptr<Module> module);
    virtual ~OperatorResolver();

    /// Resolves operators.
    ///
    /// module: The AST to resolve.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(expression::UnresolvedOperator* i) override;
    void visit(Variable* i) override;
    void visit(type::UnknownElementType* i) override;

private:
    shared_ptr<Module> _module;
    std::list<expression::UnresolvedOperator*> _unknowns;
};
}
}

#endif

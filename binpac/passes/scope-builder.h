
#ifndef BINPAC_PASSES_SCOPE_BUILDER_H
#define BINPAC_PASSES_SCOPE_BUILDER_H

#include <ast/pass.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {

class CompilerContext;

namespace passes {

/// Populates the scopes in the AST from its declarations.
class ScopeBuilder : public ast::Pass<AstInfo>
{
public:
    /// Constructor.
    ///
    /// context: The context the AST is part of.
    ScopeBuilder(CompilerContext* context);
    virtual ~ScopeBuilder();

    /// Populates an AST's scopes based on the declarations found in there.
    ///
    /// module: The AST to process.
    ///
    /// Returns: True if no error were encountered.
    bool run(shared_ptr<ast::NodeBase> ast) override;

 protected:
    void visit(declaration::Variable* v) override;
    void visit(declaration::Type* t) override;
    void visit(declaration::Constant* t) override;
    void visit(declaration::Function* t) override;
    void visit(declaration::Hook* t) override;

private:
    CompilerContext* _context;
    shared_ptr<Scope> _checkDecl(Declaration* decl);
    void _populateHookScope(shared_ptr<Hook> hook);
};

}

}

#endif

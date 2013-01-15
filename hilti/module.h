
#ifndef HILTI_MODULE_H
#define HILTI_MODULE_H

#include <ast/module.h>

#include "common.h"
#include "id.h"
#include "function.h"
#include "statement.h"
#include "type.h"

namespace hilti {

namespace passes {
    class CFG;
    class Liveness;
}

class CompilerContext;

/// AST node for a top-level module.
class Module : public ast::Module<AstInfo>
{
public:
    /// Constructor.
    ///
    /// ctx: The compiler context the module is part of.
    ///
    /// id: A non-scoped ID with the module's name.
    ///
    /// path: A file system path associated with the module. For imports,
    /// this must be given as it's used to track modules already loaded.
    ///
    /// l: Associated location.
    Module(shared_ptr<CompilerContext> ctx, shared_ptr<ID> id, const string& path = "-", const Location& l=Location::None);

    /// Returns the module's compiler context.
    shared_ptr<CompilerContext> compilerContext() const;

    /// Returns the module's execution context if any has been declared, or
    /// null if not. Note that the result is only defined after the module has
    /// been finalized.
    shared_ptr<type::Context> executionContext() const;

    /// Returns the module's control flow graph. Note that this will return
    /// null if the pass has not yet been run by the CompilerContext.
    shared_ptr<passes::CFG> cfg() const;

    /// Returns the module's liveness analysis. Note that this will return
    /// null if the pass has not yet been run by the CompilerContext.
    shared_ptr<passes::Liveness> liveness() const;

    ACCEPT_VISITOR_ROOT();

protected:
    friend class CompilerContext;

    /// Sets control- and data flow passes that have run on the module.
    /// Normally only called from the CompilerContext.
    void setPasses(shared_ptr<passes::CFG> cfg, shared_ptr<passes::Liveness> liveness);

private:
    shared_ptr<CompilerContext> _context;
    shared_ptr<passes::CFG> _cfg = nullptr;
    shared_ptr<passes::Liveness> _liveness = nullptr;
};

}

#endif

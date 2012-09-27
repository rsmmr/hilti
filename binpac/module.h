///
/// A BinPAC++ compile-time unit.
///

#ifndef BINPAC_MODULE_H
#define BINPAC_MODULE_H

#include <ast/module.h>
#include <ast/visitor.h>

#include "common.h"

namespace binpac {

class CompilerContext;

/// AST node for a top-level module.
class Module : public ast::Module<AstInfo>
{
public:
    /// Constructor.
    ///
    /// ctx: The context this module is part of.
    ///
    /// id: A non-scoped ID with the module's name.
    ///
    /// path: A file system path associated with the module.
    ///
    /// l: Associated location.
    Module(CompilerContext* ctx, shared_ptr<ID> id, const string& path = "-", const Location& l=Location::None);

    CompilerContext* context() const;

    ACCEPT_VISITOR_ROOT();

private:
    CompilerContext* _context;
};

}

#endif

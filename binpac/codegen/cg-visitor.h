
#ifndef BINPAC_CODEGEN_CG_VISITOR_H
#define BINPAC_CODEGEN_CG_VISITOR_H

#include <ast/visitor.h>
#include <hilti/ast-info.h>

#include "codegen.h"

namespace binpac {

namespace codegen {

/// Base class for visitors that the code generator uses for its work.
template <typename Result = int, typename Arg1 = int, typename Arg2 = int>
class CGVisitor : public ast::Visitor<AstInfo, Result, Arg1, Arg2> {
public:
    /// Constructor.
    ///
    /// cg: The code generator the visitor is used by.
    ///
    /// logger_name: An identifying name passed on the Logger framework.
    CGVisitor(CodeGen* cg, const string& logger_name)
    {
        _codegen = cg;

        if ( cg ) {
            this->forwardLoggingTo(cg);
            this->setLoggerName(logger_name);
        }
    }

    /// Returns the code generator this visitor is attached to.
    CodeGen* cg() const
    {
        return _codegen;
    }

    /// Returns the current HILTI block builder, retrieved from the code
    /// generator. This is just a shortcut for calling CodeGen::builder().
    ///
    /// Returns: The current HILTI builder.
    shared_ptr<hilti::builder::BlockBuilder> builder() const
    {
        assert(_codegen);
        return _codegen->builder();
    }

    /// Returns the current HILTI module builder, retrieved from the code
    /// generator. This is just a shortcut for calling CodeGen::builder().
    ///
    /// Returns: The current HILTI builder.
    shared_ptr<hilti::builder::ModuleBuilder> moduleBuilder() const
    {
        assert(_codegen);
        return _codegen->moduleBuilder();
    }

private:
    CodeGen* _codegen;
};
}
}

#endif

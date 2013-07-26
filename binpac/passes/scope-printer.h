
#ifndef BINPAC_PASSES_SCOPE_PRINTER_H
#define BINPAC_PASSES_SCOPE_PRINTER_H

#include <ast/pass.h>

#include "../common.h"
#include "../ast-info.h"

namespace binpac {
namespace passes {

/// Dumps out the contents of all scopes in an AST. This is mainly for
/// debugging.
class ScopePrinter : public ast::Pass<AstInfo>
{
public:
    /// Constructor.
    ///
    /// out: The stream to print to.
    ScopePrinter(std::ostream& out);

    virtual ~ScopePrinter();

    /// Dumps out the contents of all scopes in an AST.
    ///
    /// module: The AST which's scopes to dump
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(statement::Block* b) override;
    void visit(type::unit::Item * i) override;

private:
    std::ostream& _out;
};

}

}

#endif

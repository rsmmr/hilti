
#ifndef BINPAC_PASSES_NORMALIZER_H
#define BINPAC_PASSES_NORMALIZER_H

#include <map>

#include <ast/pass.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {
namespace passes {

/// Normalizes properties AST nodes.
class Normalizer : public ast::Pass<AstInfo>
{
public:
    /// Constructor.
    Normalizer();
    virtual ~Normalizer();

    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(type::unit::Item* i);
    void visit(type::unit::item::Field* f);
    void visit(type::unit::item::field::Container* c);
};

}
}

#endif

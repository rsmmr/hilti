
#ifndef BINPAC_PASSES_NORMALIZER_H
#define BINPAC_PASSES_NORMALIZER_H

#include <map>

#include <ast/pass.h>

#include "../ast-info.h"
#include "../common.h"

namespace binpac {
namespace passes {

/// Normalizes properties AST nodes.
class Normalizer : public ast::Pass<AstInfo> {
public:
    /// Constructor.
    Normalizer();
    virtual ~Normalizer();

    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    void visit(declaration::Type* t) override;
    void visit(declaration::Variable* t) override;
    void visit(type::unit::Item* i) override;
    void visit(type::unit::item::Field* f) override;
    void visit(type::unit::item::field::Container* c) override;

    void visit(binpac::expression::operator_::unit::Input* i) override;
    void visit(binpac::expression::operator_::unit::Offset* i) override;
    void visit(binpac::expression::operator_::unit::SetPosition* i) override;

    void visit(binpac::Expression* i) override;
    void visit(binpac::expression::operator_::unit::TryAttribute* i) override;

    void visit(statement::Return* r) override;
};
}
}

#endif

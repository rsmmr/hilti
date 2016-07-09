
#ifndef BINPAC_CONSTANTCOERCER_H
#define BINPAC_CONSTANTCOERCER_H

#include <ast/constant-coercer.h>

#include "common.h"

namespace binpac {

/// Visitor implementating the check whether a constant of one type can be
/// coerced into that of another. This class should not be used directly. The
/// frontend is expression::Constant::coerceTo.
class ConstantCoercer : public ast::ConstantCoercer<AstInfo> {
public:
    void visit(constant::Integer* i) override;
    void visit(constant::Tuple* t) override;
    void visit(constant::Optional* t) override;
};
}

#endif

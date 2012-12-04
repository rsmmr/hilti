
#ifndef BINPAC_COERCER_H
#define BINPAC_COERCER_H

#include <ast/coercer.h>

#include "common.h"
#include "type.h"

namespace binpac {

/// Visitor implementating the check whether an expression of one type can be
/// coerced into that of another. This class should not be used directly. The
/// frontend is Expression::canCoerceTo.
class Coercer : public ast::Coercer<AstInfo>
{
public:
   void visit(type::Integer* i);
};

}

#endif

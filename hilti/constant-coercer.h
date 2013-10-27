
#ifndef HILTI_CONSTANTCOERCER_H
#define HILTI_CONSTANTCOERCER_H

#include <ast/constant-coercer.h>

#include "common.h"
#include "constant.h"
#include "type.h"
#include "visitor-interface.h"

namespace hilti {

/// Visitor implementating the check whether a constant of one type can be
/// coerced into that of another. This class should not be used directly. The
/// frontend is expression::Constant::coerceTo.
class ConstantCoercer : public ast::ConstantCoercer<AstInfo>
{
public:
   void visit(constant::Address* i) override;
   void visit(constant::Integer* i) override;
   void visit(constant::Reference* r) override;
   void visit(constant::Tuple* t) override;
   void visit(constant::CAddr* t) override;
};


}

#endif

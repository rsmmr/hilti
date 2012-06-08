
#ifndef HILTI_COERCER_H
#define HILTI_COERCER_H

#include "ast/ast.h"

#include "common.h"
#include "type.h"
#include "visitor-interface.h"

namespace hilti {

/// Visitor implementating the check whether an expression of one type can be
/// coerced into that of another. This class should not be used directly. The
/// frontend is Expression::canCoerceTo.
class Coercer : public ast::Coercer<AstInfo>
{
public:
   void visit(type::Address* t);
   void visit(type::Integer* i);
   void visit(type::Reference* r);
   void visit(type::Iterator* i);
   void visit(type::Tuple* t);
};

}

#endif

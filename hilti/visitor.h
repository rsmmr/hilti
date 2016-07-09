
#ifndef HILTI_VISITOR_H
#define HILTI_VISITOR_H

#include <ast/visitor.h>

#include "common.h"
#include "visitor-interface.h"

namespace hilti {

/// Base class for HILTI visitors.
template <typename Result = int, typename Arg1 = int, typename Arg2 = int>
class Visitor : public ast::Visitor<AstInfo, Result, Arg1, Arg2> {
};
}

#endif

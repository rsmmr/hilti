
#ifndef HILTI_COMMON_H
#define HILTI_COMMON_H

#include <memory>
#include <string>

#include <ast/location.h>
#include <ast/logger.h>
#include <ast/rtti.h>

// This forward-declares all our node types, as specified by nodes.decl.
#include <hilti/autogen/visitor-types.h>

// This forward-declares all the auto-generated expression::operator_::*
// classes.
#include <hilti/autogen/instructions-declare.h>

#include "ast-info.h"

using std::string;
using std::unique_ptr;
using std::shared_ptr;

namespace hilti {
using ast::Location;
using ast::InternalError;

typedef std::list<string> path_list;
typedef std::list<string> string_list;
};

#endif

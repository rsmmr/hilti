
#ifndef HILTI_COMMON_H
#define HILTI_COMMON_H

#include <memory>
#include <string>

#include <ast/location.h>
#include <ast/logger.h>

#include "ast-info.h"

using std::string;
using std::unique_ptr;
using std::shared_ptr;

using ast::Location;
using ast::InternalError;

using ast::as;

namespace hilti {
    typedef std::list<string> path_list;
};

#endif

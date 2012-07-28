
#ifndef BINPAC_COMMON_H
#define BINPAC_COMMON_H

#include <memory>
#include <string>
#include <list>

#include "autogen/config.h"

#include <ast/location.h>
#include <ast/logger.h>

// This forward-declares all our node types.
#include "autogen/visitor-types.h"
#include "ast-info.h"

using std::string;
using std::unique_ptr;
using std::shared_ptr;

using ast::Location;
using ast::Logger;

namespace binpac {
    typedef std::list<shared_ptr<Attribute>> attribute_list;
    typedef std::list<shared_ptr<Declaration>> declaration_list;
    typedef std::list<shared_ptr<Expression>> expression_list;
    typedef std::list<shared_ptr<Variable>> variable_list;
    typedef std::list<shared_ptr<Statement>> statement_list;
    typedef std::list<shared_ptr<ID>> id_list;
    typedef std::list<shared_ptr<Type>> type_list;
    typedef std::list<shared_ptr<Hook>> hook_list;
    typedef std::list<shared_ptr<type::unit::Item>> unit_item_list;
    typedef std::list<shared_ptr<type::function::Parameter>> parameter_list;
    typedef std::list<string> string_list;
};

#endif

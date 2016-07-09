
#ifndef BINPAC_AST_INFO_H
#define BINPAC_AST_INFO_H

#include "visitor-interface.h"

namespace binpac {

struct AstInfo;

/// A shared pointer to an AST node. Note that with two pointers pointing to
/// the same node, both will get updated if we change one to point somewhere
/// else.
template <typename T>
using node_ptr = typename ast::node_ptr<T>;

/// Base class for all BinPAC++ AST nodes.
typedef ast::Node<AstInfo> Node;

class Coercer;
class ConstantCoercer;
class Scope;
class VisitorInterface;

/// Declares types for the AST library.
struct AstInfo {
    typedef binpac::Node node;
    typedef binpac::Expression scope_value;
    typedef binpac::VisitorInterface visitor_interface;
    typedef binpac::Type type;
    typedef binpac::Scope scope;
    typedef binpac::Coercer coercer;
    typedef binpac::Constant constant;
    typedef binpac::Ctor ctor;
    typedef binpac::ConstantCoercer constant_coercer;
    typedef binpac::Declaration declaration;
    typedef binpac::Expression expression;
    typedef binpac::Function function;
    typedef binpac::ID id;
    typedef binpac::Module module;
    typedef binpac::Statement statement;
    typedef binpac::Variable variable;

    typedef binpac::expression::Coerced coerced_expression;
    typedef binpac::expression::Constant constant_expression;
    typedef binpac::expression::Ctor ctor_expression;
    typedef binpac::expression::List list_expression;

    typedef binpac::statement::Block block;
    typedef binpac::statement::Block body_statement;

    typedef binpac::type::Any any_type;
    typedef binpac::type::Void void_type;
    typedef binpac::type::Block block_type;
    typedef binpac::type::Function function_type;
    typedef binpac::type::Module module_type;
    typedef binpac::type::OptionalArgument optional_type;
    typedef binpac::type::TypeType type_type;
    typedef binpac::type::Unknown unknown_type;
    typedef binpac::type::function::Parameter parameter;
    typedef binpac::type::function::Result result;
};
}

#endif

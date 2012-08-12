
#ifndef HILTI_AST_INFO_H
#define HILTI_AST_INFO_H

#include "visitor-interface.h"

namespace hilti {

struct AstInfo;

/// A shared pointer to an AST node. Note that with two pointes pointing to
/// the same node, both will get updated if we change one to point somewhere
/// else.
template<typename T>
using node_ptr = typename ast::node_ptr<T>;

/// Base class for all HILTI AST nodes.
typedef ast::Node<AstInfo> Node;

class Coercer;
class ConstantCoercer;
class VisitorInterface;

/// Declares types for the AST library.
struct AstInfo {
    typedef hilti::Coercer coercer;
    typedef hilti::Constant constant;
    typedef hilti::Ctor ctor;
    typedef hilti::ConstantCoercer constant_coercer;
    typedef hilti::Declaration declaration;
    typedef hilti::Expression expression;
    typedef hilti::Function function;
    typedef hilti::ID id;
    typedef hilti::Module module;
    typedef hilti::Node node;
    typedef hilti::Scope scope;
    typedef hilti::Statement statement;
    typedef hilti::Type type;
    typedef hilti::Variable variable;
    typedef hilti::VisitorInterface visitor_interface;

    typedef hilti::expression::Coerced coerced_expression;
    typedef hilti::expression::Constant constant_expression;
    typedef hilti::expression::Ctor ctor_expression;
    typedef hilti::expression::List list_expression;

    typedef hilti::statement::Block block;
    typedef hilti::statement::Block body_statement;

    typedef hilti::type::Any any_type;
    typedef hilti::type::OptionalArgument optional_type;
    typedef hilti::type::Block block_type;
    typedef hilti::type::Function function_type;
    typedef hilti::type::Module module_type;
    typedef hilti::type::TypeType type_type;
    typedef hilti::type::Unknown unknown_type;
    typedef hilti::type::function::Parameter parameter;
    typedef hilti::type::function::Result result;

    typedef hilti::Expression scope_value;
};

}

#endif

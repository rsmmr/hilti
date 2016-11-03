
#ifndef SPICY_AST_INFO_H
#define SPICY_AST_INFO_H

#include "visitor-interface.h"

namespace spicy {

struct AstInfo;

/// A shared pointer to an AST node. Note that with two pointers pointing to
/// the same node, both will get updated if we change one to point somewhere
/// else.
template <typename T>
using node_ptr = typename ast::node_ptr<T>;

/// Base class for all Spicy AST nodes.
typedef ast::Node<AstInfo> Node;

class Coercer;
class ConstantCoercer;
class Scope;
class VisitorInterface;

/// Declares types for the AST library.
struct AstInfo {
    typedef spicy::Node node;
    typedef spicy::Expression scope_value;
    typedef spicy::VisitorInterface visitor_interface;
    typedef spicy::Type type;
    typedef spicy::Scope scope;
    typedef spicy::Coercer coercer;
    typedef spicy::Constant constant;
    typedef spicy::Ctor ctor;
    typedef spicy::ConstantCoercer constant_coercer;
    typedef spicy::Declaration declaration;
    typedef spicy::Expression expression;
    typedef spicy::Function function;
    typedef spicy::ID id;
    typedef spicy::Module module;
    typedef spicy::Statement statement;
    typedef spicy::Variable variable;

    typedef spicy::expression::Coerced coerced_expression;
    typedef spicy::expression::Constant constant_expression;
    typedef spicy::expression::Ctor ctor_expression;
    typedef spicy::expression::List list_expression;

    typedef spicy::statement::Block block;
    typedef spicy::statement::Block body_statement;

    typedef spicy::type::Any any_type;
    typedef spicy::type::Void void_type;
    typedef spicy::type::Function function_type;
    typedef spicy::type::Module module_type;
    typedef spicy::type::OptionalArgument optional_type;
    typedef spicy::type::TypeType type_type;
    typedef spicy::type::Unknown unknown_type;
    typedef spicy::type::function::Parameter parameter;
    typedef spicy::type::function::Result result;
};
}

#endif
